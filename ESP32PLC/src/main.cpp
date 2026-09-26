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
#include "Modules/ModuleManager.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password
int BuffSize = 0;

void setup() {
  SystemStart();
   delay(1);
  Log(DEBUG,">> SystemStart done");
  pinMode(16, INPUT);
  LEDBoot();
  Log(DEBUG,">> QueryLocalDevice");
  QueryLocalDevice();
  DisplayLog(GetWiFiMode() != 0 ? "NVS: OK" : "NVS: defaults");
  Log(DEBUG,">> DigitalStart");
  DigitalStart();
  Log(DEBUG,">> IOStart");
  IOStart();
  DisplayLog("WiFi: Connecting...");
  Log(DEBUG,">> SetupWiFi");
  SetupWiFi();   // retries STA 3x then falls back to AP internally
  Log(DEBUG,">> setup_ota");
  setup_ota();
  if (GetWiFiMode() == WIFI_AP_MODE) {
    DisplayLog("WiFi: AP Mode");
  } else {
    char _wip[48];
    snprintf(_wip, sizeof(_wip), "WiFi: %s", GetIPStr().c_str());
    DisplayLog(_wip);
  }
  delay(1);
  Log(DEBUG,">> InitSensors");
  InitSensors();
  DisplayLog(GetMQTTEnabled() ? "MQTT: Enabled" : "MQTT: Disabled");
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
    String apSsid = GetSanitizedHostname();
    DisplaySetAPMode(true, apSsid.c_str());
    DisplayAPInfo(apSsid.c_str());
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

// ── Loop timing (shown on the web Log tab via /api/mem) ─────────────────────
// Max is since the last read (reset by LoopStatsRead) plus an all-time peak.
static uint32_t _loopMaxUs = 0, _loopPeakUs = 0;
static uint64_t _loopSumUs = 0;
static uint32_t _loopCount = 0;
static uint32_t _loopWinStartMs = 0;

void LoopStatsRead(uint32_t* maxUs, uint32_t* peakUs, uint32_t* avgUs, uint32_t* perSec) {
  uint32_t winMs = millis() - _loopWinStartMs;
  *maxUs  = _loopMaxUs;
  *peakUs = _loopPeakUs;
  *avgUs  = _loopCount ? (uint32_t)(_loopSumUs / _loopCount) : 0;
  *perSec = winMs ? (uint32_t)((uint64_t)_loopCount * 1000 / winMs) : 0;
  _loopMaxUs = 0; _loopSumUs = 0; _loopCount = 0; _loopWinStartMs = millis();
}

void loop() {
  uint32_t _t0 = micros();
  WiFiRecoveryLoop();
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
  SendLocalIO();   // publishes only on change (+ hourly heartbeat) — cheap every loop
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
    modules.mqttPublish();
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
  // Loop work time — measured before delay(1) so the idle yield isn't counted
  uint32_t _dt = micros() - _t0;
  if (_dt > _loopMaxUs)  _loopMaxUs  = _dt;
  if (_dt > _loopPeakUs) _loopPeakUs = _dt;
  _loopSumUs += _dt; _loopCount++;
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

