#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "enums.h"

#define DEFAULT_ADDRESS 0x00 // Default unassigned address (general call)
#define MAX_RESPONSE_SIZE 31 // Maximum response data size (32 - 1 for length byte)

int ledPin = LED_BUILTIN;
int ledState = LOW;
bool identifyMode = false;
unsigned long previousMillis = 0;
const long interval = 250;

int valvePin = 2;
bool valveActive = false;

JsonDocument doc;
uint8_t currentAddress = DEFAULT_ADDRESS;
bool addressAssigned = false;
volatile DeviceStatus currentStatus = STATUS_UNINITIALIZED;
volatile DeviceAction currentAction = DEVICE_SLEEP;

void sendResponse(const char *data)
{
  uint8_t dataLength = strlen(data);
  if (dataLength > MAX_RESPONSE_SIZE)
    dataLength = MAX_RESPONSE_SIZE;

  // Send length as first byte
  Wire.write(dataLength);
  // Send the actual data
  Wire.write((uint8_t *)data, dataLength);
}

void sendCurrentMoisture()
{
  // Read analog value from A0 (0-1023 range)
  int moistureValue = analogRead(A0);
  doc["moisture"] = moistureValue;
  char moistureBuffer[50];
  serializeJson(doc, moistureBuffer);
  sendResponse(moistureBuffer);
}

void handleAction(DeviceAction action)
{
  switch (action)
  {
  case DEVICE_STATUS:
    Serial.println("Sending STATUS response");
    sendCurrentMoisture();
    break;
  case DEVICE_ACTIVATE:
    Serial.println("Starting irrigation (simulated)");
    sendResponse("Irrigation started");
    currentStatus = STATUS_ACTIVE;
    break;
  case DEVICE_DEACTIVATE:
    Serial.println("Stopping irrigation (simulated)");
    sendResponse("Irrigation stopped");
    currentStatus = STATUS_STANDBY;
    break;
  case DEVICE_IDENTIFY:
    Serial.println("Entering identify mode (simulated)");
    identifyMode = true;
    sendResponse("Identifying");
    break;
  case DEVICE_SLEEP:
    Serial.println("Entering sleep mode (simulated)");
    identifyMode = false;
    currentStatus = STATUS_STANDBY;
    sendResponse("Sleeping");
    break;
  default:
    Serial.println("Unknown action received");
    sendResponse("Unknown action");
    break;
  }
}

void requestEvent()
{
  if (currentStatus == STATUS_UNINITIALIZED)
  {
    Wire.write(STATUS_UNINITIALIZED);
    return;
  }

  handleAction(currentAction);
}

void receiveEvent(int numBytes)
{
  if (numBytes < 1)
  {
    return;
  }

  currentAction = (DeviceAction)Wire.read();

  if (currentAction == DEVICE_ASSIGN_ADDRESS)
  {
    uint8_t newAddress = Wire.read();
    currentAddress = newAddress;
    addressAssigned = true;

    Serial.print("Address assigned: 0x");
    Serial.println(newAddress, HEX);

    // Reinitialize I2C with new address
    Wire.end();
    Wire.begin(currentAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    currentStatus = STATUS_STANDBY;
    return;
  }

  handleAction(currentAction);
}

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);
  Serial.begin(115200);
  Serial.println("I2C Slave starting...");

  Wire.begin(DEFAULT_ADDRESS);  // Start with general call address
  Wire.onReceive(receiveEvent); // register receive event handler
  Wire.onRequest(requestEvent); // register request event handler

  Serial.println("Waiting for address assignment...");
}

void loop()
{
  if (currentStatus == STATUS_ACTIVE && !valveActive)
  {
    digitalWrite(valvePin, HIGH);
    digitalWrite(ledPin, HIGH);
    valveActive = true;
  }
  else if (currentStatus == STATUS_STANDBY && valveActive)
  {
    digitalWrite(valvePin, LOW);
    digitalWrite(ledPin, LOW);
    valveActive = false;
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    if (identifyMode)
    {
      digitalWrite(ledPin, ledState);
    }
    else
    {
      digitalWrite(ledPin, LOW);
    }
  }
}