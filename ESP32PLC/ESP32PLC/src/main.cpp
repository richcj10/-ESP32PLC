#include "Functions.h"
#include "Display/Oled.h"
#include "MQTT.h"
#include "Sensors.h"
#include "Webportal.h"
#include "Display/Display.h"
#include "HAL/DeviceConfig.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "HAL/Digital/Digital.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  SystemStart();
  ClientIdCreation();
  SaveResetReason();
  FileStstemStart();
  QueryLocalDevice();
  DisplaySetup(TFT);
  //GPIOStart();
  setupMode();
  SetupWiFi();
  SystemStart();
  InitSensors();
  WiFiStart();
  //MQTTStart();
  //WebStart();
  Serial.println("Setup Done!");
  DisplayCenterClear();
  Serial.println(WiFi.getHostname());
  DisplayTimeoutReset(); //This allows the display to be shown for 10 seconds afer reboot. 
  //GPIOStart();
  pinMode(MP1INPUT, INPUT);
}

void loop() {
  MqttLoop();
  //DisplayManager();
  ScanUserInput();
  SyncLoop();
  WebHandel();
}


