#include "StatusLED.h"
#include <Arduino.h>
#include <algorithm>
#include <Adafruit_NeoPixel.h>

#define PIN 3 
#define NUMPIXELS 1

char Mode = 0;
char LEDAnaimation = 0;

long LEDRefreshRate = 0;
long LEDUpdateInterval = 100;
unsigned long LEDcurrentMillis = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void StatusLEDStart(){
  pixels.begin();
  pixels.setBrightness(50);
  pixels.show(); // Initialize all pixels to 'off'
}

void LEDBoot(){
    //strip.SetPixelColor(0, green);
    pixels.setPixelColor(0, pixels.Color(50, 0, 0));
    pixels.show();
    delay(200);
    pixels.setPixelColor(0, pixels.Color(0, 50, 0));
    pixels.show();
    delay(200);
    pixels.setPixelColor(0, pixels.Color(0, 0, 50));
    pixels.show();
}

void SetLEDStatus(char type, char rate){
  Mode = type;
  LEDUpdateInterval = rate;
}



void LEDUpdate(){
  LEDcurrentMillis = millis();
  if (LEDcurrentMillis - LEDRefreshRate >= LEDUpdateInterval) {
    // save the last time you blinked the LED
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
    pixels.setPixelColor(0, pixels.Color(0, 0, 10));
    break;
  case 1:
    pixels.setPixelColor(0, pixels.Color(0, 0, 20));
    break;
  case 2:
    pixels.setPixelColor(0, pixels.Color(0, 0, 30));
    break;
  case 3:
    pixels.setPixelColor(0, pixels.Color(0, 0, 40));
    break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 4){
    LEDAnaimation = 0;
  }
  pixels.show();
}

void NormalFcn(){
  switch (LEDAnaimation){
    case 0:
      pixels.setPixelColor(0, pixels.Color(0, 10, 0));
      break;
    case 1:
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      break;
    default:
      break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 1){
    LEDAnaimation = 0;
  }
  pixels.show();
}

void MQTTFcn(){
  switch (LEDAnaimation){
  case 0:
    pixels.setPixelColor(0, pixels.Color(0, 10, 10));
    break;
  case 1:
    pixels.setPixelColor(0, pixels.Color(0, 0, 10));
    break;
  case 2:
    pixels.setPixelColor(0, pixels.Color(0, 10, 10));
    break;
  case 3:
    pixels.setPixelColor(0, pixels.Color(10, 0, 0));
    break;
  }
  LEDAnaimation++;
  if(LEDAnaimation > 4){
    LEDAnaimation = 0;
  }
  pixels.show();
}


// #ifdef __AVR__
//   #include <avr/power.h>
// #endif

// #define PIN 6

// // Parameter 1 = number of pixels in strip
// // Parameter 2 = Arduino pin number (most are valid)
// // Parameter 3 = pixel type flags, add together as needed:
// //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
// //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
// //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
// //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
// //   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

// // IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// // pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// // and minimize distance between Arduino and first pixel.  Avoid connecting
// // on a live circuit...if you must, connect GND first.

// void setup() {
//   // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
//   #if defined (__AVR_ATtiny85__)
//     if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
//   #endif
//   // End of trinket special code

//   strip.begin();
//   strip.setBrightness(50);
//   strip.show(); // Initialize all pixels to 'off'
// }
