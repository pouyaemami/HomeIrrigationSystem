#include "Web.h"
#include <Arduino.h>
#include <Wire.h>
#include "enums.h"

extern uint8_t knownDevices[];
extern uint8_t deviceCount;
extern uint8_t maxI2cBuffer;

extern float temperature;
extern float humidity;

// Helper function to extract query parameter value from URL
String getQueryParam(String url, String paramName)
{
  int paramIndex = url.indexOf(paramName + "=");
  if (paramIndex == -1)
  {
    return ""; // Parameter not found
  }

  int valueStart = paramIndex + paramName.length() + 1;
  int valueEnd = url.indexOf('&', valueStart);
  if (valueEnd == -1)
  {
    valueEnd = url.indexOf(' ', valueStart); // Space marks end of URL
  }

  return url.substring(valueStart, valueEnd);
}

void sendDeviceCommand(String addressParam, DeviceAction action)
{
  if (addressParam.length() > 0)
  {
    // Convert hex string to byte
    uint8_t targetAddress = (uint8_t)strtol(addressParam.c_str(), NULL, 16);
    Wire.beginTransmission(targetAddress);
    Wire.write(action);
    Wire.endTransmission();
  }
  else
  {
    Serial.println("Error: No address provided for START command");
  }
}

void printWeb(WiFiClient &client)
{
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

          // Ping known devices and display moisture data
          listConnectedDevices(client);
          listTemperatureHumidity(client);

          // The HTTP response ends with another blank line:
          client.println();
          // break out of the while loop:
          break;
        }
        else
        { // if you got a newline, then process the complete line
          // Check for commands only when we have a complete line
          if (currentLine.startsWith("GET /START"))
          {
            String addressParam = getQueryParam(currentLine, "address");
            sendDeviceCommand(addressParam, DEVICE_ACTIVATE);
          }
          else if (currentLine.startsWith("GET /STOP"))
          {
            String addressParam = getQueryParam(currentLine, "address");
            sendDeviceCommand(addressParam, DEVICE_DEACTIVATE);
          }
          else if (currentLine.startsWith("GET /IDENTIFY"))
          {
            String addressParam = getQueryParam(currentLine, "address");
            sendDeviceCommand(addressParam, DEVICE_IDENTIFY);
          }
          else if (currentLine.startsWith("GET /SLEEP"))
          {
            String addressParam = getQueryParam(currentLine, "address");
            sendDeviceCommand(addressParam, DEVICE_SLEEP);
          }

          currentLine = ""; // clear currentLine
        }
      }
      else if (c != '\r')
      {                   // if you got anything else but a carriage return character,
        currentLine += c; // add it to the end of the currentLine
      }
    }
  }
  // close the connection:
  client.stop();
  Serial.println("client disconnected");
}

void listTemperatureHumidity(WiFiClient &client)
{
  client.print("<h2>Current Environmental Data</h2>");
  client.print("<ul>");
  client.print("<li><pre>Temperature: ");
  client.print(temperature);
  client.print(" &deg;C</pre></li>");
  client.print("<li><pre>Humidity: ");
  client.print(humidity);
  client.print(" %</pre></li>");
  client.print("</ul>");
}

void listConnectedDevices(WiFiClient &client)
{
  client.print("<h2>Connected I2C Devices Moisture Readings</h2>");

  if (deviceCount > 0)
  {
    client.print("<h3>Device Moisture Readings</h3>");

    client.print("<ul>");
    for (uint8_t i = 0; i < deviceCount; i++)
    {
      byte address = knownDevices[i];

      // Send DEVICE_STATUS message to the device
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

          client.print("<li><pre>Device 0x");
          if (address < 16)
          {
            client.print("0");
          }
          client.print(address, HEX);
          client.print(": ");
          client.print(response);
          client.print("</pre>");
          client.print("<a href=\"/START?address=" + String(address, HEX) + "\">Start</a>");
          client.print(" | ");
          client.print("<a href=\"/STOP?address=" + String(address, HEX) + "\">Stop</a>");
          client.print(" | ");
          client.print("<a href=\"/IDENTIFY?address=" + String(address, HEX) + "\">Identify</a>");
          client.print(" | ");
          client.print("<a href=\"/SLEEP?address=" + String(address, HEX) + "\">Sleep</a>");
          client.print("</li>");
        }
      }
    }
    client.print("</ul>");
  }
  else
  {
    client.print("No devices connected<br>");
  }
}