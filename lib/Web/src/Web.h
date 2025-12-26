#ifndef WEB_H
#define WEB_H

#include <WiFi101.h>
#include "DeviceManagement.h"

extern float temperature;
extern float humidity;

void printWeb(WiFiClient &client, DeviceManagement &deviceManager);

void listTemperatureHumidity(WiFiClient &client);

void listConnectedDevices(WiFiClient &client, DeviceManagement &deviceManager);

// Helper function to extract query parameter value
uint8_t getQueryParam(String url, String paramName);

#endif
