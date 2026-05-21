#include "Functions.h"
#include "Display/Oled.h"
#include "MQTT.h"
#include "Sensors.h"

#include "Display/Display.h"
#include "Display/UIPages.h"
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
#include "Remote/FwUpdater.h"
#include "Display/TFT.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password
int BuffSize = 0;

void setup() {
  yield();
  Log(DEBUG,">> SystemStart");
  SystemStart();
  yield();
  Log(DEBUG,">> SystemStart done");
  Log(ERROR,"Startup Complete");
  pinMode(16, INPUT);
  LEDBoot();
  Log(DEBUG,">> FileStstemStart");
  if(FileStstemStart()){
    DisplayLog(" FS OK ");
    delay(1000);
  }
  else{
    DisplayLog(" FS ERROR ");
    delay(1000);
  }
  yield();
  Log(DEBUG,">> QueryLocalDevice");
  QueryLocalDevice();
  Log(DEBUG,">> DigitalStart");
  DigitalStart();
  Log(DEBUG,">> IOStart");
  IOStart();
  yield();
  DisplayLog(" Connecting to WiFi...");
  Log(DEBUG,">> SetupWiFi");
  yield();
  SetupWiFi();   // retries STA 3x then falls back to AP internally
  yield();
  Log(DEBUG,">> setup_ota");
  setup_ota();
  DisplayLog(GetIPStr().c_str());
  delay(1000);
  Log(DEBUG,">> InitSensors");
  InitSensors();
  yield();
  Log(DEBUG,">> MQTTStart");
  MQTTStart();
  Log(DEBUG,">> WebStart");
  WebStart();
  Log(DEBUG,">> fwUpdater.init");
  fwUpdater.init();
  Log(DEBUG,">> SetLEDStatus");
  SetLEDStatus(NORMAL,1000);
  Serial.println("Setup Done!");
  Log(DEBUG,">> DisplayTimeoutReset");
  DisplayTimeoutReset();
  Log(DEBUG,">> WiFiMode check");
  if (GetWiFiMode() == WIFI_AP_MODE) {
    DisplaySetAPMode(true, GetHostName().c_str());
    DisplayAPInfo(GetHostName().c_str());
  } else {
    DisplaySetAPMode(false, nullptr);
    DisplayClear();
    Log(DEBUG,">> UIPageInit");
    UIPageInit();
    Log(DEBUG,">> UIPageDraw");
    UIPageDraw();
  }
  Log(DEBUG,">> Setup complete");
}

int l =0;
unsigned long LastSendTime, LastSendTime2, LastSendTime3 = 0;
char TSChannel = 1;
char Prect = 0;

unsigned long LastSendTimeCH1, LastSendTimeCH2, LastSendTimeCH3, LastSendTimeCH4, LastSendTimeCH5 =0;
char CH1FT, CH2FT, CH3FT, CH4FT, CH5FT = 0;

void loop() {
  CaptivePortalLoop();
  ScanIO();
  RemoteRun();
  UIUpdateLoop();
  SensorUpdateLoop();
  
  //
  //Serial.println("loop");
  //GetJoystickPrint(GetJoyStickPos());
  MqttLoop();
  
  //ScanUserInput();
  SyncLoop();
/*   if(millis() - LastSendTime > 3000){
    LastSendTime = millis();
    //SetCHFire(4, 1);
    //ReadRemoteCurrent();
    //ReadRemoteTemp();
    //GetRemoteTemp(10);
  } */
  
  if(millis() - LastSendTime2 > 1500){
    LastSendTime2 = millis();
    SendRemoteDevices();
    SendLocalIO();
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
  //RunFW();

/*   if(GetCHFire(1) == 1){
    if(CH1FT == 0){
      digitalWrite(40,HIGH);  //CH1 
      LastSendTimeCH1 = millis();
      CH1FT = 1;
    }
    if(millis() - LastSendTimeCH1 > 2500){
      SetCHFire(1, 0);
      digitalWrite(40,LOW);  //CH1 
      CH1FT = 0;
    }
  }
  if(GetCHFire(2) == 1){
    if(CH2FT == 0){
      digitalWrite(41,HIGH);  //CH1 
      LastSendTimeCH2 = millis();
      CH2FT = 1;
    }
    if(millis() - LastSendTimeCH2 > 2500){
      SetCHFire(2, 0);
      digitalWrite(41,LOW);  //CH1 
      CH2FT = 0;
    }
  }
  if(GetCHFire(3) == 1){
    if(CH3FT == 0){
      digitalWrite(39,HIGH);  //CH1 
      LastSendTimeCH3 = millis();
      CH3FT = 1;
    }
    if(millis() - LastSendTimeCH3 > 2500){
      SetCHFire(3, 0);
      digitalWrite(39,LOW);  //CH1 
      CH3FT = 0;
    }
  }
  if(GetCHFire(4) == 1){
    if(CH4FT == 0){
      digitalWrite(42,HIGH);  //CH1 
      LastSendTimeCH4 = millis();
      CH4FT = 1;
    }
    if(millis() - LastSendTimeCH4 > 2500){
      SetCHFire(4, 0);
      digitalWrite(42,LOW);  //CH1 
      CH4FT = 0;
    }
  }
  if(GetCHFire(5) == 1){
    if(CH5FT == 0){
      digitalWrite(38,HIGH);  //CH1 
      LastSendTimeCH5 = millis();
      CH5FT = 1;
    }
    if(millis() - LastSendTimeCH5 > 2500){
      SetCHFire(5, 0);
      digitalWrite(38,LOW);  //CH1 
      CH5FT = 0;
    }
  } */
}

