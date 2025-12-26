#include <Wire.h>
#include "DeviceManagement.h"
#include <config.h>
#include "enums.h"

#define DEFAULT_ADDRESS 0x00       // Default address for unassigned devices
#define BASE_ASSIGNED_ADDRESS 0x08 // Starting address for assignment
#define MAX_DEVICES 10             // Maximum number of devices to track
#define MAX_I2C_BUFFER 32          // Maximum I2C buffer size

uint8_t maxI2cBuffer = MAX_I2C_BUFFER;
uint8_t nextAvailableAddress = BASE_ASSIGNED_ADDRESS;
uint8_t knownDevices[MAX_DEVICES];
uint8_t deviceCount = 0;

void DeviceManagement::setup()
{
  // Initialize Wire library
  Wire.begin();

  Serial.println("\nI2C Scanner with Address Assignment");
  DeviceManagement::discoverDevices();

  Serial.println("\nChecking for unassigned devices...");

  for (int i = 0; i < MAX_DEVICES; i++)
  {
    // First, assign addresses to any unassigned devices
    Wire.beginTransmission(DEFAULT_ADDRESS);
    byte error = Wire.endTransmission();

    if (error == 0)
    {
      // Check if it's a new device
      delay(10);
      Wire.requestFrom(DEFAULT_ADDRESS, MAX_I2C_BUFFER);

      if (Wire.available())
      {
        DeviceStatus status = (DeviceStatus)Wire.read();
        if (status == STATUS_UNINITIALIZED)
        {
          Serial.println("Found unassigned device at default address");
          assignAddress(DEFAULT_ADDRESS);
        }
      }
    }
  }

  Serial.println("\nDevice discovery complete!");
}

void DeviceManagement::discoverDevices()
{
  byte error, address;

  Serial.println("Scanning for devices...");

  for (address = 1; address < 127; address++)
  {
    // Skip address 0x60 (ATECC508A CryptoAuthentication chip)
    if (address == 0x60)
      continue;

    // Skip default unassigned address
    if (address == DEFAULT_ADDRESS)
      continue;

    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);

      // Check if device already exists in the list
      bool alreadyKnown = false;
      for (uint8_t i = 0; i < deviceCount; i++)
      {
        if (knownDevices[i] == address)
        {
          alreadyKnown = true;
          break;
        }
      }

      // Add to known devices list if not already there
      if (!alreadyKnown && deviceCount < MAX_DEVICES)
      {
        knownDevices[deviceCount] = address;
        deviceCount++;
        Serial.print("Total devices: ");
        Serial.println(deviceCount);
      }
    }
  }

  Serial.print("Total devices discovered: ");
  Serial.println(deviceCount);
}

void DeviceManagement::assignAddress(byte defaultAddr)
{
  Serial.print("Assigning address 0x");
  if (nextAvailableAddress < 16)
    Serial.print("0");
  Serial.print(nextAvailableAddress, HEX);
  Serial.print(" to new device... ");

  Wire.beginTransmission(defaultAddr);
  Wire.write(DEVICE_ASSIGN_ADDRESS);
  Wire.write(nextAvailableAddress);
  Wire.endTransmission();

  delay(50); // Give device time to reconfigure

  Serial.println("address assigned successfully!");

  // Add to known devices list
  if (deviceCount < MAX_DEVICES)
  {
    knownDevices[deviceCount] = nextAvailableAddress;
    deviceCount++;
  }

  nextAvailableAddress++;
}

DeviceBuffer DeviceManagement::getConnectedDevices()
{
  uint8_t *devices = new uint8_t[deviceCount];
  for (uint8_t i = 0; i < deviceCount; i++)
  {
    devices[i] = knownDevices[i];
  }

  return {devices, deviceCount};
}

void DeviceManagement::sendDeviceCommand(uint8_t address, DeviceAction action)
{
  Serial.print("Triggering action ");
  Serial.print(action);
  Serial.print(" on device 0x");
  if (address < 16)
    Serial.print("0");
  Serial.println(address, HEX);

  Wire.beginTransmission(address);
  Wire.write(action);
  Wire.endTransmission();
}

String DeviceManagement::getDeviceData(uint8_t address)
{
  Serial.print("Requesting data from device 0x");
  if (address < 16)
    Serial.print("0");
  Serial.println(address, HEX);

  Wire.beginTransmission(address);
  Wire.write(DEVICE_STATUS);
  byte error = Wire.endTransmission();

  if (error == 0)
  {
    // Request response from the device
    delay(10);
    Wire.requestFrom(address, maxI2cBuffer);

    if (Wire.available())
    {
      uint8_t dataLength = Wire.read();
      String response = "";

      for (uint8_t j = 0; j < dataLength && Wire.available(); j++)
      {
        response += (char)Wire.read();
      }

      Serial.print("Received data from device 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.print(": ");
      Serial.println(response);
      return response;
    }
    else
    {
      Serial.println("No data available from device.");
    }
  }
  else
  {
    Serial.println("Error communicating with device.");
  }

  return String("");
}