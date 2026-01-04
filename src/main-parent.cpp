#include <Arduino.h>
#include <SPI.h>
#include <WiFi101.h>
#include <DHT.h>
#include <NTPClient.h>
#include <RTCZero.h>
#include "time.h"

#include "config.h"
#include "Web.h"
#include "MDNS.h"
#include "DeviceManagement.h"

WiFiServer server(80);
WiFiClient client = server.available();
MDNS mdns;
DeviceManagement deviceManager;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
RTCZero rtc;

#define DHTPIN 0      // Pin which is connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;

void setRTCFromNTP()
{
  timeClient.update();
  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti;
  ti = localtime(&rawtime);

  rtc.setTime(ti->tm_hour, ti->tm_min, ti->tm_sec);
  rtc.setDate(ti->tm_mday, ti->tm_mon + 1, ti->tm_year - 100);
}

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

  // start the NTP client
  timeClient.begin();
  rtc.begin();

  setRTCFromNTP();
}

void loop()
{
  timeClient.update();
  mdns.poll();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  client = server.available();
  printWeb(client, deviceManager);
}