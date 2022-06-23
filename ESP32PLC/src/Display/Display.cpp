#include "Display.h"
#include <WiFi.h>
#include "Oled.h"
#include "TFT.h"
#include "Define.h"
#include "HAL/Digital/Digital.h"
#include "MQTT.h"
#include "Devices/Joystick.h"

char DisplayMode = 1;
char Displaytype = 0;
char ScreenShow = 0;
unsigned long TimeReading = 0;
unsigned long LastTimeReading = 0;
unsigned long LastDisplayUpdate = 0;
char DisplaySleepEn = 1;

long DisplayRefreshRate = 0;
long DisplayUpdateInterval = 1000;
unsigned long DisplaycurrentMillis = 0;

void DisplaySetup(){
  switch (Displaytype){
  case TFT:
    TFTInit();
    ledcSetup(0, LEDC_BASE_FREQ, 12);
    ledcAttachPin(LED_PIN, 0);
    TFTLogo();
    break;
  case OLED:
    Serial.println("OLED Config");
    OLEDInit();
    break;
  default:
    Serial.println("No Display Config");
    break;
  }
}

void DisplayWiFiRSSI(){
  switch (Displaytype){
  case TFT:
    TFTWiFiSignal(0);
    break;
  case OLED:
    WiFiCheckRSSI(0);
    break;
  }
}

char LastMQTT = 0;

void CheckMQTTCon(char overide){
  if(GetMQTTStatus() == 1){
    if((LastMQTT != 1) || (overide == 1)){
      DisplayMQTT(1);
      LastMQTT = 1;
    }
  }
  else{
    if((LastMQTT != 2) || (overide == 1)){
      DisplayMQTT(0);
      LastMQTT = 2;
    }
  }
}

void DisplayManager(){
  if(DisplayMode == 1){
    //DeviceIDDisplay();
    DisplayTHBar();
    DisplayWiFiRSSI();
    CheckMQTTCon(0);   //Don't force MQTT value to be "refreshed" here...
    DislaySaver();
  }
  else{
    DisplayClear();
  }
  if ((millis() - LastDisplayUpdate) > 500){
    DisplayUserHandeler();
    LastDisplayUpdate = millis();
  }
}

void DisplayTHBar(){
  switch (Displaytype){
    case TFT:
      TFTTHBar();
      break;
    case OLED:
      OLEDTHBar();
      break;
  }
}

void DisplayMQTT(char Mode){
  switch (Displaytype){
    case TFT:
      TFTMQTTIconSet(Mode);
      break;
    case OLED:
      OLEDMQTTIconSet(Mode);
      break;
  }
}

void DislaySaver(){
  if(DisplaySleepEn == 1){
    if ((millis() - LastTimeReading) > SREENTIMEOUT){
      DisplayMode = 0;
      DisplayClear();
      if(Displaytype == TFT){
        DisplayBrightnes(0);
      }
    }
  }
}

void DisplayClear(){
  switch (Displaytype){
    case TFT:
      TFTDisplayClear();
      break;
    case OLED:
      OledDisplayClear();
      break;
  }
}

void DisplayUserHandeler(void){
  if(GetJoyStickSelect()){
    DisplayTimeoutReset();
    if(DisplayMode == 0){
      if(Displaytype == TFT){
        DisplayBrightnes(25);
      }
      //DeviceIDDisplay();
      DisplayTHBar();
      WiFiCheckRSSI(1);   //We need to force display refresh here, because we are redrawing the blank display at this moment. 
      CheckMQTTCon(1);
      DisplayMode = 1;
    }
    //DisplaySwitchCase();
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

char k =0;

void DisplayWiFiConnect(){
  DisplaycurrentMillis = millis();
  if (DisplaycurrentMillis - DisplayRefreshRate >= 250) {
    DisplayRefreshRate = DisplaycurrentMillis;
    switch (Displaytype){
      case TFT:
        TFTWiFiConnect(k);
        k++;
        if(k>3){
          k = 0;
        }
        //TFTWiFiSignal();
        break;
      case OLED:
        Serial.println("OLED Config");
        break;
      default:
        Serial.println("No Display Config");
        break;
    }
  }
}\

char LastWiFiSig = 0;

void WiFiCheckRSSI(char overide){
  long Rssi = WiFi.RSSI()*-1;
  if(Rssi > HIGHRSSI){
    if((LastWiFiSig != 1) || (overide == 1)){
      WiFiStreanthDisplay(1);
      LastWiFiSig = 1;
    }
  }
  else if((Rssi < HIGHRSSI) && (Rssi > LOWRSSI)){
    if((LastWiFiSig != 2) || (overide == 1)){
      WiFiStreanthDisplay(2);
      LastWiFiSig = 2;
    }
  }
  else if(Rssi < LOWRSSI){
    if((LastWiFiSig != 3)  || (overide == 1)){
      WiFiStreanthDisplay(3);
      LastWiFiSig = 3;
    }
  }
}

void DisplayTimeoutReset(){
  LastTimeReading = millis();
}

void DispalySleepControl(char value){
  DisplaySleepEn = value;
}

void DispalyConfigSet(char value){
  Displaytype = value;
}

void DisplayLogo(){
  switch (Displaytype){
  case TFT:
    TFTLogoDisplay();
    break;
  case OLED:
    Serial.println("OLED Config");
    break;
  default:
    Serial.println("No Display Config");
    break;
  }
}

void DisplayLog(const char *Text){
  switch (Displaytype){
  case TFT:
    TFTLog(Text);
    break;
  case OLED:
    Serial.println("OLED Config");
    break;
  }
}

void DisplayBrightnes(char Brightness){
  switch (Displaytype){
    case TFT:
      // calculate duty, 4095 from 2 ^ 12 - 1
      //uint32_t duty = (4095 / 255) *Brightness;
      ledcWrite(0, ((4095 / 255) *Brightness));
      break;
    case OLED:
      Serial.println("OLED Config");
      break;
    default:
      Serial.println("No Display Config");
      break;
  }
}
