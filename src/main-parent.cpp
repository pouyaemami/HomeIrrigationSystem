#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi101.h>
#include <DHT.h>

#include "wifi_credentials.h"
#include "enums.h"
#include "Web.h"

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS; // the Wifi radio's status

WiFiServer server(80);
int ledPin = LED_BUILTIN;

#define DHTPIN 0      // Pin which is connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;

#define DEFAULT_ADDRESS 0x00       // Default address for unassigned devices
#define BASE_ASSIGNED_ADDRESS 0x08 // Starting address for assignment
#define MAX_DEVICES 10             // Maximum number of devices to track
#define MAX_I2C_BUFFER 32          // Maximum I2C buffer size

uint8_t maxI2cBuffer = MAX_I2C_BUFFER;
uint8_t nextAvailableAddress = BASE_ASSIGNED_ADDRESS;
uint8_t knownDevices[MAX_DEVICES];
uint8_t deviceCount = 0;

void receiveResponse()
{
  if (!Wire.available())
    return;

  // Read length byte (first byte)
  uint8_t dataLength = Wire.read();

  // Read the actual data
  String response = "";
  for (uint8_t i = 0; i < dataLength && Wire.available(); i++)
  {
    char c = Wire.read();
    response += c;
  }

  if (response.length() > 0)
  {
    Serial.print("  Received: ");
    Serial.print(response);
    Serial.println("");
  }
}

void assignAddress(byte defaultAddr)
{
  Serial.print("Assigning address 0x");
  if (nextAvailableAddress < 16)
    Serial.print("0");
  Serial.print(nextAvailableAddress, HEX);
  Serial.println(" to new device...");

  Wire.beginTransmission(defaultAddr);
  Wire.write(DEVICE_ASSIGN_ADDRESS);
  Wire.write(nextAvailableAddress);
  Wire.endTransmission();

  delay(50); // Give device time to reconfigure

  Serial.println("  Address assigned successfully");

  // Add to known devices list
  if (deviceCount < MAX_DEVICES)
  {
    knownDevices[deviceCount] = nextAvailableAddress;
    deviceCount++;
  }

  nextAvailableAddress++;
}

void discoverDevices()
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
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

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
      }
    }
  }

  Serial.print("Total devices discovered: ");
  Serial.println(deviceCount);
}

void printData()
{
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
}

void setup()
{
  Wire.begin(); // join i2c bus as master

  while (!Serial)
    ;

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 1 second for connection:
    delay(1000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

  Serial.println("----------------------------------------");
  printData();
  Serial.println("----------------------------------------");

  server.begin();

  Serial.begin(115200);
  while (!Serial)
    delay(10);
  Serial.println("\nI2C Scanner with Address Assignment");
  Serial.println("\nChecking for unassigned devices...");

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

  // Discover all devices and build the device list
  Serial.println();
  discoverDevices();
  Serial.println("\nDevice discovery complete!");

  dht.begin();
}

void loop()
{
  WiFiClient client = server.available();

  if (client)
  {
    printWeb(client);
  }

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}