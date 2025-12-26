#ifndef MDNS_H
#define MDNS_H

#include <WiFi101.h>
#include <WiFiMDNSResponder.h>

class MDNS
{
private:
  WiFiMDNSResponder mdnsResponder;
  void printWiFiStatus();

public:
  void setup(WiFiServer &server, String mdnsName);
  void poll();
};

#endif