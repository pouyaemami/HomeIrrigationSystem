#include <Arduino.h>
#include <WiFi101.h>

#include "MDNS.h"

void MDNS::setup(WiFiServer &server, String mdnsName)
{
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true)
      ;
  }

  // Start in provisioning mode:
  //  1) This will try to connect to a previously associated access point.
  //  2) If this fails, an access point named "wifi101-XXXX" will be created, where XXXX
  //     is the last 4 digits of the boards MAC address. Once you are connected to the access point,
  //     you can configure an SSID and password by visiting http://wifi101/
  WiFi.beginProvision();

  while (WiFi.status() != WL_CONNECTED)
  {
    // wait while not connected

    // blink the led to show an unconnected status
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  server.begin();

  // Start the MDNS responder
  if (!mdnsResponder.begin(mdnsName.c_str()))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
      ;
  }
  Serial.println("MDNS responder started");
  Serial.print("You can now connect to http://");
  Serial.print(mdnsName);
  Serial.println(".local");

  MDNS::printWiFiStatus();
}

void MDNS::poll()
{
  // Poll the MDNS responder to process incoming requests
  mdnsResponder.poll();
}

void MDNS::printWiFiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi 101 Shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}