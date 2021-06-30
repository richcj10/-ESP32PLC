#include "Functions.h"
#include "Oled.h"
#include "MQTT.h"
#include "Sensors.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  GPIOInit();
  SystemStart();
  InitSensors();
  ClientIdCreation();
  DisplayInit();
  WiFiStart();
  MQTTStart();
  Serial.println("Setup Done!");
  DisplayCenterClear();
  Serial.println(WiFi.getHostname());
}

void loop() {
  MqttLoop();
  SyncLoop();
}
