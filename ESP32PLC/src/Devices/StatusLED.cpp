#include "StatusLED.h"
#include <Arduino.h>
#include <FastLED.h>
#include "Devices/Log.h"

#define PIN 3 
#define NUMPIXELS 1
char Brightness = 50;
CRGB leds[NUMPIXELS];
char color = 0;
char Mode = 0;
char LEDAnaimation = 0;

long LEDRefreshRate = 0;
long LEDUpdateInterval = 100;
unsigned long LEDcurrentMillis = 0;

void StatusLEDStart(){
  FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMPIXELS);  // GRB ordering is assumed
}

void LEDBoot(){
    FastLED.showColor(CRGB(255, 0, 100));
    delay(200);
    FastLED.showColor(CRGB(0, 255, 100));
    delay(200);
    FastLED.showColor(CRGB(0, 0, 255));
}

void SetLEDStatus(char type, int rate){
  Mode = type;
  LEDUpdateInterval = rate;
}



void LEDUpdate(){
  LEDcurrentMillis = millis();
  if (LEDcurrentMillis - LEDRefreshRate >= LEDUpdateInterval) {
    // save the last time you blinked the LED
    Log(DEBUG,"LED Update, Mode = %d\r\n",Mode);
    LEDRefreshRate = LEDcurrentMillis;
    switch (Mode){
      case WIFI_CONNECTING:
        WiFiFcn();
        break;
      case MQTT_CONNECTING:
        WiFiFcn();
        break;
      case NORMAL:
        NormalFcn();
        break;
      default:
        break;
    }
  }
}

char GetStatus(){
    return 0;
}


void WiFiFcn(){
  switch (LEDAnaimation){
  case 0:
    FastLED.showColor(CRGB(0, 0, 100));
    break;
  case 1:
    FastLED.showColor(CRGB(0, 0, 150));
    break;
  case 2:
    FastLED.showColor(CRGB(0, 0, 200));
    break;
  case 3:
    FastLED.showColor(CRGB(0, 0, 255));
    break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 4){
    LEDAnaimation = 0;
  }
}

void NormalFcn(){
  switch (LEDAnaimation){
    case 0:
      FastLED.showColor(CRGB(0, 255, 0));
      break;
    case 1:
      FastLED.showColor(CRGB(0, 0, 0));
      break;
    default:
      break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 1){
    LEDAnaimation = 0;
  }
}

void MQTTFcn(){
  switch (LEDAnaimation){
  case 0:
    FastLED.showColor(CRGB(0, 0, 0));
    break;
  case 1:
    FastLED.showColor(CRGB(0, 0, 10));
    break;
  case 2:
    FastLED.showColor(CRGB(0, 10, 0));
    break;
  case 3:
    FastLED.showColor(CRGB(10, 0, 0));
    break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 4){
    LEDAnaimation = 0;
  }
}

void LEDUpdate(char Value) {
  color = Value;
  FastLED.showColor(CHSV(color, 255, Brightness)); 
}

void LEDBrightness(char Value) {
  Brightness = Value;
  FastLED.showColor(CHSV(color, 255, Brightness)); 
}

char LEDGetValue(){
  return color;
}