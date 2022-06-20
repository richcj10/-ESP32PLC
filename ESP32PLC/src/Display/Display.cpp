#include "Display.h"
#include <WiFi.h>
#include "Oled.h"
#include "TFT.h"
#include "Define.h"
#include "HAL/Digital/Digital.h"


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

void DisplayManager(){
  if(DisplayMode == 1){
    DeviceIDDisplay();
    //DisplayTHBar();
    DisplayWiFiRSSI();
    //CheckMQTTCon(0);   //Don't force MQTT value to be "refreshed" here...
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
      //DeviceIDDisplay();
      //DisplayTHBar();
      //WiFiCheckRSSI(1);   //We need to force display refresh here, because we are redrawing the blank display at this moment. 
      //CheckMQTTCon(1);
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
    //TFTLogoDisplay();
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
