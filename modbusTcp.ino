#include "ModbusTCP.h"

const int ledPin = LED_BUILTIN;

ModbusTCP *modbus = nullptr;

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  modbus = new ModbusTCP();
  modbus->init("you_ssid", "your_password", 502);
  modbus->begin();
}

void loop() {
  modbus->iterate();
}
