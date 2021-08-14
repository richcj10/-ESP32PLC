#include "Menu.h"
#include <WiFi.h>
#include "Oled.h"
#include "Define.h"

char DisplayMode = 1;
char ScreenShow = 0;
unsigned long TimeReading = 0;
unsigned long LastTimeReading = 0;
char DisplaySleepEn = 1;


void DisplayTimeoutReset(){
  LastTimeReading = millis();
}

void DislaySaver(){
  if(DisplaySleepEn == 1){
    if(DisplayMode == 1){
      unsigned long TimeReading = millis();
      if (TimeReading - LastTimeReading > 10000){
        DisplayMode = 0;
      }
    }
  }
}

void DisplayManager(){
  if(DisplayMode == 1){
    DeviceIDDisplay();
    DisplayTHBar();
    WiFiCheckRSSI(0);  //Don't force the RSSI to be updated.....
    CheckMQTTCon(0);   //Don't force MQTT value to be "refreshed" here...
  }
  else{
    FullDisplayClear();
  }
  ButtonHandeler();
}

void ButtonHandeler(){
  if(digitalRead(USER_SW) == 0){
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
    case 0:    // your hand is on the sensor
      DisplayCenterChestTemp();
      ScreenShow++;
      break;
    case 1:    // your hand is close to the sensor
      DisplayCenterInput();
      ScreenShow++;
      break;
    case 2:    // your hand is close to the sensor
       DisplayCenterOutput();
       ScreenShow++;
      break;
    case 3:    // your hand is close to the sensor
       DisplayCenterOutput();
       ScreenShow++;
      break;
    default:
      //This is a catch for errors
      ScreenShow = 0; //This allows for a circular buffer on display items. 
      break;
  }
  Serial.print("Screen = ");
  Serial.println(int(ScreenShow));
}

void DispalySleepControl(char value){
  DisplaySleepEn = value;
}
