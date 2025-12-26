#include <Arduino.h>
#include <SPI.h>
#include <WiFi101.h>
#include <DHT.h>

#include "config.h"
#include "Web.h"
#include "MDNS.h"
#include "DeviceManagement.h"

// int status = WL_IDLE_STATUS; // the Wifi radio's status

WiFiServer server(80);
WiFiClient client = server.available();
MDNS mdns;
DeviceManagement deviceManager;

int ledPin = LED_BUILTIN;

#define DHTPIN 0      // Pin which is connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial)
    ;

  // Setup MDNS
  mdns.setup(server, "irrigation-system");

  // Setup Device Management
  deviceManager.setup();

  dht.begin();
}

void loop()
{
  mdns.poll();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  client = server.available();
  printWeb(client, deviceManager);
}