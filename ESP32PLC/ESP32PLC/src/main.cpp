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
  pinMode(2, OUTPUT);
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  SystemStart();
  ClientIdCreation();
  SaveResetReason();
  FileStstemStart();
  QueryLocalDevice();
  DispalyConfigSet(TFT);
  DisplaySetup();
  DisplayLogo();
  //GPIOStart();
  setupMode();
  //SetupWiFi();
  //InitSensors();
  //WiFiStart();
  //MQTTStart();
  //WebStart();
  Serial.println("Setup Done!");
  //DisplayCenterClear();
  //DisplayTimeoutReset(); //This allows the display to be shown for 10 seconds afer reboot. 
  //GPIOStart();
  //pinMode(MP1INPUT, INPUT);
}

void loop() {
  //MqttLoop();
  //DisplayManager();
  //ScanUserInput();
  //SyncLoop();
  //WebHandel();
  DisplayWiFiSignal();
  delay(1000);
  digitalWrite(2,HIGH);
  delay(1000);
  digitalWrite(2,LOW);
}

