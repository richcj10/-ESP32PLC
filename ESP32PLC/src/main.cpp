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

#include "Display/TFT.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password
int BuffSize = 0;

void setup() {
  LogSetup(NOTIFY,1);
  SaveResetReason();
  ClientIdCreation();
  SystemStart();
  Log(NOTIFY,"Startup Complete");
  pinMode(2, OUTPUT);
  pinMode(16,INPUT);
  pinMode(40,OUTPUT); // Relay Control 
  LEDBoot();
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
  //setupMode();
  //Serial.print("SiSensor = ");
  //Serial.println(Si7021checkID());
  DisplayLog(" Connecting to WiFi...");
  if(!SetupWiFi()){
    DisplayLog("Connection Failed..Setting up AP");
    SetWiFisetupMode(WIFI_AP_MODE);
    SetupWiFi();
  }
  setup_ota();
  DisplayLog(GetIPStr().c_str());
  delay(1000);
  InitSensors();
  MQTTStart();
  WebStart();
  //DisplayCenterClear();
  //GPIOStart();
  //pinMode(MP1INPUT, INPUT);
  //I2CScan();
  SetLEDStatus(NORMAL,1000);
  Serial.println("Setup Done!");
  DisplayTimeoutReset();//This allows the display to be shown for 10 seconds afer reboot.
  TFTBargraph(1);
}

int l =0;
unsigned long LastSendTime, LastSendTime2, LastSendTime3 = 0;
char TSChannel = 1;
char Send,Prect = 0;

void loop() {
  //
  
  RemoteRun();
  
  
  UIUpdateLoop();
  SensorUpdateLoop();
  
  //
  //Serial.println("loop");
  //GetJoystickPrint(GetJoyStickPos());
  MqttLoop();
  
  //ScanUserInput();
  SyncLoop();
  if(millis() - LastSendTime > 3000){
    LastSendTime = millis();
    //ReadRemoteCurrent();
    //ReadRemoteTemp();
    //GetRemoteTemp(10);
  }
  
  if(millis() - LastSendTime2 > 3000){
    LastSendTime2 = millis();
    if(Send == 0) SendOutsideEnvoroment();
    if(Send == 1) SendRemoteRTD();
    if(Send == 2) SendRemoteCurrentSense();
    Send++;
    if(Send > 3){
      Send = 0;
    }
    //ReadRemoteWeather();
  }

  if(millis() - LastSendTime3 > 200){
    LastSendTime3 = millis();
    Prect++;
    if(Prect > 100){
      //TFTDisplayClear();
      //TFTBargraph(1);
      Prect = 0;
    }
    //TFTBargraphUpdate(Prect);
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

