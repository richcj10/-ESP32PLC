#include "Functions.h"
#include "Display/Oled.h"
#include "MQTT.h"
#include "Sensors.h"

#include "Display/Display.h"
#include "HAL/DeviceConfig.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "HAL/Digital/Digital.h"
#include "HAL/Com/I2C.h"
#include "Devices/StatusLED.h"
#include "Devices/JoyStick.h"
#include "Webportal.h"
#include "Devices/Log.h"
#include "Remote/MasterController.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password
int BuffSize = 0;

void setup() {
  LogSetup(LOG,1);
  SaveResetReason();
  ClientIdCreation();
  SystemStart();
  Log(NOTIFY,"Startup Complete");
  pinMode(2, OUTPUT);
  pinMode(16,INPUT);
  LEDBoot();
  //if(FileStstemStart()){
  //  DisplayLog(" FS OK ");
  //  delay(1000);
  //}
  //else{
  //  DisplayLog(" FS ERROR ");
  //  delay(1000);
  //}
  QueryLocalDevice();


  //GPIOStart();
  //setupMode();
  //Serial.print("SiSensor = ");
  //Serial.println(Si7021checkID());
  DisplayLog(" Connecting to WiFi...");
  if(!SetupWiFi()){
    DisplayLog("Connection Failed..Setting up AP");
    SetWiFisetupMode(WIFI_AP_MODE);
    SetupWiFi();
  }
  DisplayLog(GetIPStr().c_str());
  delay(1000);
  InitSensors();
  MQTTStart();
  //WebStart();
  //DisplayCenterClear();
  //GPIOStart();
  //pinMode(MP1INPUT, INPUT);
  //I2CScan();
  SetLEDStatus(NORMAL,1000);
  Serial.println("Setup Done!");
  DisplayTimeoutReset();//This allows the display to be shown for 10 seconds afer reboot.
  delay(100); 
  Serial.print("Buff Size = ");
  Serial.print(Serial1.availableForWrite());
}

int l =0;
unsigned long LastSendTime = 0;
char TSChannel = 1;

void loop() {
  //
  
  RemoteRun();
  
  
  //UIUpdateLoop();
  //SensorUpdateLoop();
  
  //
  //Serial.println("loop");
  //GetJoystickPrint(GetJoyStickPos());
  //MqttLoop();
  
  //ScanUserInput();
  //SyncLoop();
  if(millis() - LastSendTime > 500){
    LastSendTime = millis();
    ReadRemoteTemp();
    //GetRemoteTemp(10);
  }
  //DisplayWiFiSignal();
  //SetOcupyLED(0x11,200,0,0);
  //SetOcupyLED(0x10,0,0,0);
  //delay(1000);
  //SetOcupyLED(0x11,0,200,0);
  //SetOcupyLED(0x10,0,0,200);
  delay(1);
  //Serial.println(readDeviceVIN(0x10));
  //Serial.println(readDeviceVIN(0x11));
  //Serial.println(readDeviceVIN(0x20));
}

