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
#include "HAL/Com/I2C.h"
#include "Devices/StatusLED.h"
#include "Devices/JoyStick.h"
#include "Remote/MasterController.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  pinMode(2, OUTPUT);
  pinMode(16,INPUT);
  StatusLEDStart();
  LEDBoot();
  DispalyConfigSet(TFT);
  DisplaySetup();
  DisplayBrightnes(50);
  delay(1000);
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  SystemStart();
  ClientIdCreation();
  SaveResetReason();
  if(FileStstemStart()){
    DisplayLog(" FS OK ");
    delay(1000);
  }
  else{
    DisplayLog(" FS ERROR ");
    delay(1000);
  }
  QueryLocalDevice();


  //GPIOStart();
  setupMode();
  //Serial.print("SiSensor = ");
  //Serial.println(Si7021checkID());
  DisplayLog(" Connecting to WiFi...");
  SetupWiFi();
  DisplayLog(GetIPStr().c_str());
  delay(1000);
  //InitSensors();
  //WiFiStart();
  //MQTTStart();
  WebStart();
  Serial.println("Setup Done!");
  //DisplayCenterClear();
  //DisplayTimeoutReset(); //This allows the display to be shown for 10 seconds afer reboot. 
  //GPIOStart();
  //pinMode(MP1INPUT, INPUT);
  //I2CScan();
  JoyStickStart();
  MasterStart();
  SetLEDStatus(NORMAL,250);
}

int l =0;

void loop() {
  JoyStickUpdate();
  //SetStatus();
  LEDUpdate();
  //UpdateSensors();
  //Serial.print("Joystick = ");
  //Serial.println(GetJoyStickPos());
  //MqttLoop();
  //DisplayManager();
  //ScanUserInput();
  //SyncLoop();
  WebHandel();
  //DisplayWiFiSignal();
  //SetOcupyLED(0x11,200,0,0);
  //SetOcupyLED(0x10,0,0,0);
  //delay(1000);
  //SetOcupyLED(0x11,0,200,0);
  //SetOcupyLED(0x10,0,0,200);
  delay(1000);
  //Serial.println(readDeviceVIN(0x10));
  //Serial.println(readDeviceVIN(0x11));
  //Serial.println(readDeviceVIN(0x20));
}

