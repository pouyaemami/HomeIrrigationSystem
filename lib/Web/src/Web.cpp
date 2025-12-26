#include "Web.h"
#include <Arduino.h>
#include <Wire.h>
#include "enums.h"

// Helper function to extract query parameter value from URL
uint8_t getQueryParam(String url, String paramName)
{
  int paramIndex = url.indexOf(paramName + "=");
  if (paramIndex == -1)
  {
    return 0; // Parameter not found
  }

  int valueStart = paramIndex + paramName.length() + 1;
  int valueEnd = url.indexOf('&', valueStart);
  if (valueEnd == -1)
  {
    valueEnd = url.indexOf(' ', valueStart); // Space marks end of URL
  }

  return url.substring(valueStart, valueEnd).toInt();
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

void printWeb(WiFiClient &client, DeviceManagement &deviceManager)
{
  if (!client)
    return;

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
          listConnectedDevices(client, deviceManager);
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
            uint8_t addressParam = getQueryParam(currentLine, "address");
            deviceManager.sendDeviceCommand(addressParam, DEVICE_ACTIVATE);
          }
          else if (currentLine.startsWith("GET /STOP"))
          {
            uint8_t addressParam = getQueryParam(currentLine, "address");
            deviceManager.sendDeviceCommand(addressParam, DEVICE_DEACTIVATE);
          }
          else if (currentLine.startsWith("GET /IDENTIFY"))
          {
            uint8_t addressParam = getQueryParam(currentLine, "address");
            deviceManager.sendDeviceCommand(addressParam, DEVICE_IDENTIFY);
          }
          else if (currentLine.startsWith("GET /SLEEP"))
          {
            uint8_t addressParam = getQueryParam(currentLine, "address");
            deviceManager.sendDeviceCommand(addressParam, DEVICE_SLEEP);
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

void listConnectedDevices(WiFiClient &client, DeviceManagement &deviceManager)
{
  client.print("<h2>Connected I2C Devices Moisture Readings</h2>");

  DeviceBuffer devices = deviceManager.getConnectedDevices();
  if (devices.size > 0)
  {
    client.print("<h3>Device Moisture Readings</h3>");

    client.print("<ul>");
    for (uint8_t i = 0; i < devices.size; i++)
    {
      byte address = devices.data[i];
      String data = deviceManager.getDeviceData(address);

      client.print("<li><pre>Device 0x");
      if (address < 16)
      {
        client.print("0");
      }
      client.print(address, HEX);
      client.print(": ");
      client.print(data);
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
    client.print("</ul>");
  }
  else
  {
    client.print("No devices connected<br>");
  }

  // Free allocated memory
  delete[] devices.data;
}