#include "Menu.h"
#include <WiFi.h>
#include "Oled.h"
#include "Define.h"
#include "MQTT.h"

char DisplayMode = 1;
char LastMQTT = 0;
char ScreenShow = 0;
unsigned long TimeReading = 0;
unsigned long LastTimeReading = 0;


void MenuStart(){
  LastTimeReading = millis();
}

void DislaySaver(){
  if(DISPLAY_SLEEP_EN == 1){
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
    WiFiCheckRSSI();
    CheckMQTTCon();
  }
  else{
    FullDisplayClear();
  }
  ButtonHandeler();
}

void ButtonHandeler(){
  if(digitalRead(USER_SW) == 0){
    if(DisplayMode == 0){
      LastTimeReading = millis();
      LastMQTT = 0;
      LastMQTT = 0;
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
      ScreenShow = 0;
      break;
  }
  Serial.print("Screen = ");
  Serial.println(int(ScreenShow));
}

void CheckMQTTCon(){
  if(GetMQTTStatus() == 1){
    if(LastMQTT != 1){
      MQTTIconSet(1);
      LastMQTT = 1;
    }
  }
  else{
    if(LastMQTT != 2){
      MQTTIconSet(0);
      LastMQTT = 2;
    }
  }
}


char LastWiFiSig = 0;

void WiFiCheckRSSI(){
  long Rssi = WiFi.RSSI()*-1;
  if(Rssi > HIGHRSSI){
    if(LastWiFiSig != 1){
      WiFiStreanthDisplay(1);
      LastWiFiSig = 1;
    }
  }
  else if((Rssi < HIGHRSSI) && (Rssi > LOWRSSI)){
    if(LastWiFiSig != 2){
      WiFiStreanthDisplay(2);
      LastWiFiSig = 2;
    }
  }
  else if(Rssi < LOWRSSI){
    if(LastWiFiSig != 3){
      WiFiStreanthDisplay(3);
      LastWiFiSig = 3;
    }
  }
}
