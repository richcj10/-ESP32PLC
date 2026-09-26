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

// Own controller handle: _statusShow() would paint EVERY FastLED output,
// including the LED strip module. _statusShow() drives only the status pixel.
static CLEDController* _statusCtl = nullptr;
static volatile bool   _ledOverride = false;   // set while the boot-reset watcher owns the LED

static void _statusShow(const CRGB& c) {
  leds[0] = c;
  if (_statusCtl) _statusCtl->showLeds(255);
}

void StatusLEDStart(){
  _statusCtl = &FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMPIXELS);
}

void StatusLEDSet(uint8_t r, uint8_t g, uint8_t b) { _statusShow(CRGB(r, g, b)); }
void StatusLEDOverride(bool on)                     { _ledOverride = on; }

void LEDBoot(){
    _statusShow(CRGB(255, 0, 100));
    delay(200);
    _statusShow(CRGB(0, 255, 100));
    delay(200);
    _statusShow(CRGB(0, 0, 255));
}

void SetLEDStatus(char type, int rate){
  Mode = type;
  LEDUpdateInterval = rate;
}



void LEDUpdate(){
  if (_ledOverride) return;
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
  case 0: _statusShow(CRGB(0, 0, 100)); break;
  case 1: _statusShow(CRGB(0, 0, 150)); break;
  case 2: _statusShow(CRGB(0, 0, 200)); break;
  case 3: _statusShow(CRGB(0, 0, 255)); break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 4) LEDAnaimation = 0;
}

void NormalFcn(){
  switch (LEDAnaimation){
  case 0: _statusShow(CRGB(0, 255, 0)); break;
  case 1: _statusShow(CRGB(0, 0, 0));   break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 1) LEDAnaimation = 0;
}

void MQTTFcn(){
  switch (LEDAnaimation){
  case 0: _statusShow(CRGB(0, 0, 0));   break;
  case 1: _statusShow(CRGB(0, 0, 10));  break;
  case 2: _statusShow(CRGB(0, 10, 0));  break;
  case 3: _statusShow(CRGB(10, 0, 0));  break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 4) LEDAnaimation = 0;
}

void LEDUpdate(char Value) {
  color = Value;
  _statusShow(CHSV(color, 255, Brightness));
}

void LEDBrightness(char Value) {
  Brightness = Value;
  _statusShow(CHSV(color, 255, Brightness));
}

char LEDGetValue(){
  return color;
}