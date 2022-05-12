#include "Display.h"
#include <WiFi.h>
#include "Oled.h"
#include "TFT.h"
#include "Define.h"
#include "HAL/Digital/Digital.h"


char DisplayMode = 1;
char ScreenShow = 0;
unsigned long TimeReading = 0;
unsigned long LastTimeReading = 0;
unsigned long LastDisplayUpdate = 0;
char DisplaySleepEn = 1;

void DisplaySetup(char Type){
  if(Type == TFT){
    TFTInit();
  }
  else if(Type == OLED){
    Serial.println("OLED Config");
  }
  else{
    Serial.println("No Display Config");
  }
}

void DisplayManager(){
  if(DisplayMode == 1){
    DeviceIDDisplay();
    DisplayTHBar();
    WiFiCheckRSSI(0);  //Don't force the RSSI to be updated.....
    CheckMQTTCon(0);   //Don't force MQTT value to be "refreshed" here...
    DislaySaver();
  }
  else{
    FullDisplayClear();
  }
  if ((millis() - LastDisplayUpdate) > 500){
    DisplayUserHandeler();
    LastDisplayUpdate = millis();
  }
}

void DislaySaver(){
  if(DisplaySleepEn == 1){
    if ((millis() - LastTimeReading) > SREENTIMEOUT){
      DisplayMode = 0;
      FullDisplayClear();
    }
  }
}

void DisplayUserHandeler(void){
  if(GetUserSWValue()){
    DisplayTimeoutReset();
    if(DisplayMode == 0){
      DeviceIDDisplay();
      DisplayTHBar();
      WiFiCheckRSSI(1);   //We need to force display refresh here, because we are redrawing the blank display at this moment. 
      CheckMQTTCon(1);
      DisplayMode = 1;
    }
    DisplaySwitchCase();
  }
}

void DisplaySwitchCase(){
  switch (ScreenShow) {
    case 0:
      DisplayCenterChestTemp();
      ScreenShow++;
      break;
    case 1:
      DisplayCenterInput();
      ScreenShow++;
      break;
    case 2:
       DisplayCenterOutput();
       ScreenShow++;
      break;
    case 3:
       DisplayCenterOutput();
       ScreenShow++;
      break;
    default:
      ScreenShow = 0;
      break;
  }
  Serial.print("Screen = ");
  Serial.println(int(ScreenShow));
}

void DisplayTimeoutReset(){
  LastTimeReading = millis();
}

void DispalySleepControl(char value){
  DisplaySleepEn = value;
}
