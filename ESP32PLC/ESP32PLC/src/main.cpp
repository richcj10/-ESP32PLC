#include "Functions.h"
#include "Oled.h"
#include "MQTT.h"
#include "Sensors.h"
#include "Webportal.h"
#include "Menu.h"
#include "HAL/DeviceConfig.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  SystemStart();
  ClientIdCreation();
  SaveResetReason();
  FileStstemStart();
  QueryLocalDevice();
  GPIOStart();
  setupMode();
  SetupWiFi();
  //SystemStart();
  //InitSensors();
  
  //DisplayInit();
  //WiFiStart();
  //MQTTStart();
  //WebStart();
  Serial.println("Setup Done!");
  //DisplayCenterClear();
  Serial.println(WiFi.getHostname());
  //DisplayTimeoutReset(); //This allows the display to be shown for 10 seconds afer reboot. 
  //QueryLocalDevice();
  //GPIOStart();
}

void loop() {
  //MqttLoop();
  //DisplayManager();
  //SyncLoop();
  //WebHandel();
}


