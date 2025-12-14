#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi101.h>

#include "wifi_credentials.h"

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS; // the Wifi radio's status

WiFiServer server(80);
int ledPin = LED_BUILTIN;

WiFiClient client = server.available();

#define DEFAULT_ADDRESS 0x00       // Default address for unassigned devices
#define BASE_ASSIGNED_ADDRESS 0x08 // Starting address for assignment
#define MAX_DEVICES 10             // Maximum number of devices to track
#define MAX_I2C_BUFFER 32          // Maximum I2C buffer size

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

  // Send address assignment command: 'A' followed by new address
  Wire.beginTransmission(defaultAddr);
  Wire.write('A');
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

void printWEB()
{

  if (client)
  {                               // if you get a client,
    Serial.println("new client"); // print a message out the serial port
    String currentLine = "";      // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        if (c == '\n')
        { // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // create the buttons
            client.print("Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED off<br><br>");

            // Ping known devices and display moisture data
            if (deviceCount > 0)
            {
              client.print("<h3>Device Moisture Readings:</h3>");

              for (uint8_t i = 0; i < deviceCount; i++)
              {
                byte address = knownDevices[i];

                // Send moistureCheck message to the device
                Wire.beginTransmission(address);
                Wire.write("moistureCheck");
                byte error = Wire.endTransmission();

                if (error == 0)
                {
                  // Request response from the device
                  delay(10);
                  Wire.requestFrom(address, MAX_I2C_BUFFER);

                  if (Wire.available())
                  {
                    uint8_t dataLength = Wire.read();
                    String response = "";
                    for (uint8_t j = 0; j < dataLength && Wire.available(); j++)
                    {
                      response += (char)Wire.read();
                    }

                    client.print("Device 0x");
                    if (address < 16)
                      client.print("0");
                    client.print(address, HEX);
                    client.print(": ");
                    client.print(response);
                    client.print("<br>");
                  }
                }
              }
            }
            else
            {
              client.print("No devices connected<br>");
            }

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else
          { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }

        if (currentLine.endsWith("GET /H"))
        {
          digitalWrite(ledPin, HIGH);
        }
        if (currentLine.endsWith("GET /L"))
        {
          digitalWrite(ledPin, LOW);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
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

    // wait 10 seconds for connection:
    delay(10000);
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
      uint8_t dataLength = Wire.read();
      String response = "";
      for (uint8_t i = 0; i < dataLength && Wire.available(); i++)
      {
        response += (char)Wire.read();
      }

      if (response == "new")
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
  Serial.println("Starting ping loop...\n");
}

void loop()
{
  client = server.available();

  if (client)
  {
    printWEB();
  }
}