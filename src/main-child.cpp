#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>

#define DEFAULT_ADDRESS 0x00 // Default unassigned address (general call)
#define MAX_RESPONSE_SIZE 31 // Maximum response data size (32 - 1 for length byte)

JsonDocument doc;
uint8_t currentAddress = DEFAULT_ADDRESS;
bool addressAssigned = false;
String lastRequest = ""; // Track the last request type

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
  char moistureBuffer[50];            // Max 4 digits + null terminator
  serializeJson(doc, moistureBuffer); // For debugging
  // snprintf(moistureBuffer, sizeof(moistureBuffer), "%d", moistureValue);
  sendResponse(moistureBuffer);
}

void requestEvent()
{
  if (!addressAssigned)
  {
    sendResponse("new");
  }
  else if (lastRequest == "ping")
  {
    sendResponse("pong");
  }
  else if (lastRequest == "moistureCheck")
  {
    sendCurrentMoisture();
  }
  else
  {
    sendResponse("ok");
  }

  // Clear the last request
  lastRequest = "";
}

void receiveEvent(int numBytes)
{
  if (numBytes < 1)
    return;

  char command = Wire.read();

  // Address assignment command
  if (command == 'A' && numBytes == 2)
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
    return;
  }

  // Read remaining bytes as message
  String message = "";
  message += command;
  while (Wire.available())
  {
    char c = Wire.read();
    message += c;
  }

  // Handle different message types
  if (message == "ping")
  {
    Serial.println("Received ping, responding with pong");
    lastRequest = "ping";
  }
  else if (message == "moistureCheck")
  {
    Serial.println("Received moistureCheck, responding with moisture value");
    lastRequest = "moistureCheck";
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("I2C Slave starting...");

  Wire.begin(DEFAULT_ADDRESS);  // Start with general call address
  Wire.onReceive(receiveEvent); // register receive event handler
  Wire.onRequest(requestEvent); // register request event handler

  Serial.println("Waiting for address assignment...");
}

void loop()
{
  // Nothing needed in loop for I2C slave
  delay(100);
}