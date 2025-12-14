#include <Arduino.h>
#include <Wire.h>

int flowPin = 2;            // Flow sensor input pin
int waterPumpRelayPin = 3;  // Relay control pin
int waterValveRelayPin = 4; // Relay control pin

const float pulsesPerLiter = 129;      // 103.8 pulses = 1 liter
volatile unsigned long pulseCount = 0; // Track total number of pulses
volatile float waterML = 0.0;
float lastWaterML = 0.0;

void measureFlow()
{
  pulseCount++;
  // Calculate water volume in mL
  waterML = (pulseCount / pulsesPerLiter) * 1000.0;
}

void setup()
{
  pinMode(flowPin, INPUT);
  pinMode(waterPumpRelayPin, OUTPUT);
  pinMode(waterValveRelayPin, OUTPUT);
  digitalWrite(waterPumpRelayPin, LOW);  // Deactivate relay
  digitalWrite(waterValveRelayPin, LOW); // Deactivate relay
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(flowPin), measureFlow, RISING);
}

// the loop function runs over and over again forever
void loop()
{
  if (waterML != lastWaterML)
  {
    lastWaterML = waterML;
    Serial.print("Water volume: ");
    Serial.print(waterML);
    Serial.print(" mL with ");
    Serial.print(pulseCount);
    Serial.println(" pulses");
  }

  if (waterML >= 50)
  {
    digitalWrite(waterPumpRelayPin, LOW);  // Deactivate relay
    digitalWrite(waterValveRelayPin, LOW); // Deactivate relay
  }
  else
  {
    digitalWrite(waterValveRelayPin, HIGH); // Activate relay
    digitalWrite(waterPumpRelayPin, HIGH);  // Activate relay
  }
}