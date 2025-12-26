#ifndef DeviceManagement_H
#define DeviceManagement_H

#include <Arduino.h>
#include "enums.h"

struct DeviceBuffer
{
  uint8_t *data;
  size_t size;
};

class DeviceManagement
{
private:
  void assignAddress(byte defaultAddr);

public:
  void setup();
  void discoverDevices();
  DeviceBuffer getConnectedDevices();
  void sendDeviceCommand(uint8_t address, DeviceAction action);
  String getDeviceData(uint8_t address);
};

#endif