#include "Display.h"
#include <WiFi.h>
#include "Oled.h"
#include "TFT.h"
#include "Define.h"
#include "HAL/Digital/Digital.h"
#include "MQTT.h"
#include "Devices/Joystick.h"

char DisplayMode = 1;
char ModeActive = 1;
char Displaytype = 0;
char ScreenShow = 0;
unsigned long TimeReading = 0;
unsigned long DisplayOnTime = 0;
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
  if(DisplayMode == 0){  //If Display is off,
      if(GetJoyStickSelect()){ //And then we get a Joystic Select, (TODO: Make this a "movment")
        DisplayTimeoutReset(); //Reset Display Saver (New Timeout)
        if(Displaytype == TFT){  //If we have the TFT, I need to enable the back light 
          DisplayBrightnes(25);
        }
        DisplayTHBar();
        WiFiCheckRSSI(1);   //We need to force display refresh here, because we are redrawing the blank display at this moment. 
        CheckMQTTCon(1);
        DisplayID();
        DisplayMode = 1; //The Display is now "on", handel user selections
      //DeviceIDDisplay();
    }
  }
  else{
    if ((millis() - LastDisplayUpdate) > 1500){
      LastDisplayUpdate = millis();
      //DeviceIDDisplay();
      DisplayTHBar();
      WiFiCheckRSSI(0);
      CheckMQTTCon(0);   //Don't force MQTT value to be "refreshed" here...
      LastDisplayUpdate = millis();
      ModeActive = 1;
    }
    if(GetJoyStickSelect() && ModeActive){
      //Change UI
      DisplayTimeoutReset();
      ModeActive = 0;
      LastDisplayUpdate = millis();
      DisplaySwitchCase();
    }
    DisplaySaver();
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

void DisplayID(){
  switch (Displaytype){
    case TFT:
      TFTIDSet();
      break;
    case OLED:

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

void DisplaySaver(){
  if(DisplaySleepEn == 1){
    if ((millis() - DisplayOnTime) > SREENTIMEOUT){
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

void DisplaySwitchCase(){
  switch (ScreenShow) {
    case 0:
      DisplayCenterInput();
      ScreenShow++;
      break;
    case 1:
       DisplayCenterOutput();
       ScreenShow++;
      break;
    case 2:
       DisplayCenterIPInfo();
       ScreenShow++;
      break;
    default:
      ScreenShow = 0;
      break;
  }
  Serial.print("Screen = ");
  Serial.println(int(ScreenShow));
}

void DisplayCenterInput(void){
  switch (Displaytype){
    case TFT:
      TFTDisplayInputs();
      break;
    case OLED:
      //OledDisplayInputs();
      break;
  }
}

void DisplayCenterOutput(void){
  switch (Displaytype){
    case TFT:
      TFTDisplayOutputs();
      break;
    case OLED:
      //OledDisplayOutputs();
      break;
  }
}

void DisplayCenterIPInfo(void){
  switch (Displaytype){
    case TFT:
      TFTDisplayIP();
      break;
    case OLED:
      //OledDisplayIP();
      break;
  }
}

void DisplayCenterRemoteInfo(void){
  switch (Displaytype){
    case TFT:
      TFTDisplayRemote();
      break;
    case OLED:
      //OledDisplayClear();
      break;
  }
}


char k =0;

void DisplayWiFiConnect(){
  DisplaycurrentMillis = millis();
  if (DisplaycurrentMillis - DisplayRefreshRate >= 250) {
    DisplayRefreshRate = DisplaycurrentMillis;
    k++;
    if(k>3){
      k = 0;
    }
    switch (Displaytype){
      case TFT:
        TFTWiFiConnect(k);
        break;
      case OLED:
        WiFiStreanthDisplay(k);
        break;
      default:
        Serial.println("No Display Config");
        break;
    }
  }
}

char LastWiFiSig = 0;

void WiFiCheckRSSI(char overide){
  long Rssi = WiFi.RSSI()*-1;
  if(Rssi < HIGHRSSI){
    if((LastWiFiSig != 1) || (overide == 1)){
      switch (Displaytype){
        case TFT:
          TFTWiFiConnect(1);
          break;
        case OLED:
          WiFiStreanthDisplay(1);
          break;
      }
      LastWiFiSig = 1;
    }
  }
  else if((Rssi > HIGHRSSI) && (Rssi < LOWRSSI)){
    if((LastWiFiSig != 2) || (overide == 1)){
      switch (Displaytype){
        case TFT:
          TFTWiFiConnect(2);
          break;
        case OLED:
          WiFiStreanthDisplay(2);
          break;
      }
      LastWiFiSig = 2;
    }
  }
  else if(Rssi > LOWRSSI){
    if((LastWiFiSig != 3)  || (overide == 1)){
      switch (Displaytype){
        case TFT:
          TFTWiFiConnect(3);
          break;
        case OLED:
          WiFiStreanthDisplay(3);
          break;
      }
      LastWiFiSig = 3;
    }
  }
}

void DisplayTimeoutReset(){
  DisplayOnTime = millis();
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
  }
}
