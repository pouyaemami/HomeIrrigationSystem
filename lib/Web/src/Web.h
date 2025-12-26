#ifndef WEB_H
#define WEB_H

#include <WiFi101.h>

extern uint8_t knownDevices[];
extern uint8_t deviceCount;
extern uint8_t maxI2cBuffer;

extern float temperature;
extern float humidity;

void printWeb(WiFiClient &client);

void listTemperatureHumidity(WiFiClient &client);

void listConnectedDevices(WiFiClient &client);

// Helper function to extract query parameter value
String getQueryParam(String url, String paramName);

#endif
