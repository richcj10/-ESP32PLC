#include "Functions.h"
#include <rom/rtc.h>
#include "Display/Oled.h"
#include "Sensors.h"
#include "MQTT.h"
#include "Display/Display.h"
#include "HAL/Digital/Digital.h"
#include <WiFi.h>
#include "HAL/Com/I2C.h"
#include "Devices/StatusLED.h"
#include "Devices/JoyStick.h"
#include "Webportal.h"
#include "Remote/MasterController.h"
#include "Devices/Log.h"
#include "Display/TFT.h"

unsigned long lastMsg = 0;
unsigned long lastUpdate = 0;
unsigned long lastSensorScanRate = 0;
char CPUOneResetReason = 0;
char CPUTwoResetReason = 0;
String clientId = "";
char PinLastVal = 0;
int SensorScanRate = 1000;
char WiFiConnected = 0;

void print_reset_reason(RESET_REASON reason);

void SystemStart(){
  StatusLEDStart();
  Serial.begin(115200);
  Log(DEBUG,"LED Start-");
  LEDBoot();
  Log(DEBUG,"TFT Start-");
  DispalyConfigSet(TFT);
  DisplaySetup();
  DisplayBrightnes(75);
  Log(DEBUG,"RS485 Master Start");
  RemoteStart();
  Log(DEBUG,"I2C Master Start");
  I2CStart();
  Log(DEBUG,"Joystick Start");
  JoyStickStart();
}

void UIUpdateLoop(){
  JoyStickUpdate();
  LEDUpdate();
  DisplayManager();
  WebHandel();
}

void SensorUpdateLoop(){
  if (millis() - lastSensorScanRate >= SensorScanRate) {
    // save the last time you blinked the LED
    lastSensorScanRate = millis();
    UpdateSensors();
  }
}

void SyncLoop(){
  ArduinoOTA.handle();
  unsigned long now = millis();
  if (now - lastUpdate > 700){
    lastUpdate = now;
    ToggletUserLED();
    if ((WiFi.status() != WL_CONNECTED)) {
      Serial.println("Rebooting - Lost WiFi");
      delay(1000);
      ESP.restart();
    }
  }
}

String OTA;
char numb = 0;

void setup_ota() {
  ArduinoOTA.setHostname(GetClientId().c_str());
  //ArduinoOTA.setPassword(WiFiSettings.password.c_str());

  ArduinoOTA.onStart([]() {
    Log(NOTIFY_FORCE,"OTA Update!");
    delay(100);
    DisplayLog(" Getting OTA Update...");
    delay(1000);
    TFTBargraph(1);
  });
  ArduinoOTA.onEnd([]() {
    Log(NOTIFY_FORCE,"OTA Update - Complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //if(numb == 0){
    //  Serial.print(progress);
    //  Serial.print(" ");
    //  Serial.println(total);
    //}
    //numb = 1;
    //OTA = String("Progress: %d\r", (progress / (total)));
    float Precent = ((float)progress / (float)total)*100;
    Serial.print(" progress = ");
    Serial.println(progress);
    Serial.print(" total = ");
    Serial.println(total);
    Serial.print(" Precent = ");
    Serial.println(Precent);
    TFTBargraphUpdate(Precent);
    delay(10);
    //Log(NOTIFY_FORCE,"OTA");
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR){
      Log(NOTIFY_FORCE,"Auth Failed");
      return 0; //Don't restart due to password issue. 
    }
    else if (error == OTA_BEGIN_ERROR) Log(NOTIFY_FORCE,"Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Log(NOTIFY_FORCE,"Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Log(NOTIFY_FORCE,"Receive Failed");
    else if (error == OTA_END_ERROR) Log(NOTIFY_FORCE,"End Failed");
    ESP.restart(); //Just encase we lost WiFi, or got stuck, lets try a restart. 
  });

  ArduinoOTA.begin();
}

void WiFiFaiure(){
  WiFiAP(1);
  WiFiConnected = 0;
}

void WiFiOK(){
  WiFiConnected = 1;
}

unsigned long Countdown = 0;

char GetWiFiStatus(void){
  return(WiFiConnected);
}

String GetClientId(void){
  return(clientId);
}

void ClientIdCreation(void){
  byte mac[6];
  WiFi.macAddress(mac);
  clientId = "ESPPLC-";
  clientId = clientId + String(mac[1]) + String(mac[0]);
  
  Serial.print("ClientID = ");
  Serial.println(clientId);
}

void SaveResetReason(void){
  CPUOneResetReason = rtc_get_reset_reason(0);
  //print_reset_reason(rtc_get_reset_reason(0));
  CPUTwoResetReason = rtc_get_reset_reason(1);
  //print_reset_reason(rtc_get_reset_reason(1));
}

char GetResetReason(char cpucore){
  if(cpucore == 0){
    return CPUOneResetReason;
  }
  else if(cpucore == 1){
    return CPUTwoResetReason;
  }
  else{
    return 255;
  }
}


void print_reset_reason(RESET_REASON reason){
  switch ( reason){
    case 1 : 
      Serial.println ("POWERON_RESET");
      break;          /**<1,  Vbat power on reset*/
    case 3 : 
      Serial.println ("SW_RESET");
      break;               /**<3,  Software reset digital core*/
    case 4 : 
      Serial.println ("OWDT_RESET");
      break;             /**<4,  Legacy watch dog reset digital core*/
    case 5 : 
      Serial.println ("DEEPSLEEP_RESET");
      break;        /**<5,  Deep Sleep reset digital core*/
    case 6 : 
      Serial.println ("SDIO_RESET");
      break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7 : 
      Serial.println ("TG0WDT_SYS_RESET");
      break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : 
      Serial.println ("TG1WDT_SYS_RESET");
      break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : 
      Serial.println ("RTCWDT_SYS_RESET");
      break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : 
      Serial.println ("INTRUSION_RESET");
      break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : 
      Serial.println ("TGWDT_CPU_RESET");
      break;       /**<11, Time Group reset CPU*/
    case 12 : 
      Serial.println ("SW_CPU_RESET");
      break;          /**<12, Software reset CPU*/
    case 13 : 
      Serial.println ("RTCWDT_CPU_RESET");
      break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : 
      Serial.println ("EXT_CPU_RESET");
      break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : 
      Serial.println ("RTCWDT_BROWN_OUT_RESET");
      break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : 
      Serial.println ("RTCWDT_RTC_RESET");
      break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : 
      Serial.println ("NO_MEAN");
  }
}