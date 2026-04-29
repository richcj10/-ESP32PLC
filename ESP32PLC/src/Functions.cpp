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
#include "WifiControl/WifiConfig.h"

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
  LogSetup(DEBUG,1); 
  SaveResetReason();
  ClientIdCreation();
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
    if(GetWiFisetupMode() != 2){
      if ((WiFi.status() != WL_CONNECTED)) {
        Serial.println("Rebooting - Lost WiFi");
        delay(1000);
        ESP.restart();
      }
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
  clientId = clientId + String(mac[4]) + String(mac[5]);
  //Serial.print(WiFi.macAddress());
  Log(NOTIFY_FORCE,"ClientID = %s",clientId);
  //Serial.print("ClientID = ");
  //Serial.println(clientId);
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

char FWStart = 0;
char FWCH1 = 0;
char FWCH2 = 0;
char FWCH3 = 0;
char FWCH4 = 0;
char FWCH5 = 0;
int FWCH1_T = 0;
int FWCH2_T = 0;
int FWCH3_T = 0;
int FWCH4_T = 0;
int FWCH5_T = 0;

void SetFWData(char ch1,char ch2,char ch3,char ch4,char ch5,int ch1t, int ch2t,int ch3t,int ch4t,int ch5t){
  Log(NOTIFY,"CH 1 %d, CH1 Time %d\r\n",ch1,ch1t);
  Log(NOTIFY,"CH 2 %d, CH2 Time %d\r\n",ch2,ch2t);
  Log(NOTIFY,"CH 3 %d, CH3 Time %d\r\n",ch3,ch3t);
  Log(NOTIFY,"CH 4 %d, CH4 Time %d\r\n",ch4,ch4t);
  Log(NOTIFY,"CH 5 %d, CH5 Time %d\r\n",ch5,ch5t);
  FWCH1 = ch1;
  FWCH2 = ch2;
  FWCH3 = ch3;
  FWCH4 = ch4;
  FWCH5 = ch5;
  FWCH1_T = ch1t*1000;
  FWCH2_T = ch2t*1000;
  FWCH3_T = ch3t*1000;
  FWCH4_T = ch4t*1000;
  FWCH5_T = ch5t*1000;
  FWStart = 1;
}

char FirstTime = 0;
unsigned long TimeStamp = 0;
unsigned long CurrentTime = 0;

char CH1_Fire = 0;
char CH2_Fire = 0;
char CH3_Fire = 0;
char CH4_Fire = 0;
char CH5_Fire = 0;


void RunFW(){
  if(FWStart == 1){
    if(FirstTime == 0){
      TimeStamp = millis();
      FirstTime = 1;
    }
    CurrentTime = millis();
    if(FWCH1 == 1){
      if ((CurrentTime-TimeStamp) >= FWCH1_T){
        Log(NOTIFY,"CH 1 Fire\r\n");
        FWCH1 = 0;
        CH1_Fire = 1;
      }
    }
    if(FWCH2 == 1){
      if ((CurrentTime-TimeStamp) >= FWCH2_T){
        Log(NOTIFY,"CH 2 Fire\r\n");
        FWCH2 = 0;
        CH2_Fire = 1;
      }
    }
    if(FWCH3 == 1){
      if ((CurrentTime-TimeStamp) >= FWCH3_T){
        Log(NOTIFY,"CH 3 Fire\r\n");
        FWCH3 = 0;
        CH3_Fire = 1;
      }
    }
    if(FWCH4 == 1){
      if ((CurrentTime-TimeStamp) >= FWCH4_T){
        Log(NOTIFY,"CH 4 Fire\r\n");
        FWCH4 = 0;
        CH4_Fire = 1;
      }
    }
    if(FWCH5 == 1){
      if ((CurrentTime-TimeStamp) >= FWCH5_T){
        Log(NOTIFY,"CH 5 Fire\r\n");
        FWCH5 = 0;
        CH5_Fire = 1;
      }
    }
    if((FWCH1 == 0) && (FWCH2 == 0) && (FWCH3 == 0) && (FWCH4 == 0) && (FWCH5 == 0)){
      FWStart = 0;
      FirstTime = 0;
    }
  }
}

char GetCHFire(char ch){
  if (ch == 1)
  {
    return CH1_Fire;
  }
  if (ch == 2)
  {
    return CH2_Fire;
  }
  if (ch == 3)
  {
    return CH3_Fire;
  }
  if (ch == 4)
  {
    return CH4_Fire;
  }
  if (ch == 5)
  {
    return CH5_Fire;
  }
}

void SetCHFire(char ch, char data){
  if (ch == 1)
  {
    CH1_Fire = data;
  }
  if (ch == 2)
  {
    CH2_Fire = data;
  }
  if (ch == 3)
  {
    CH3_Fire = data;
  }
  if (ch == 4)
  {
    CH4_Fire = data;
  }
  if (ch == 5)
  {
    CH5_Fire = data;
  }
}