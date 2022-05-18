#include "StatusLED.h"
#include <Arduino.h>
#include <algorithm>
#include <Adafruit_NeoPixel.h>

#define PIN        3 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void StatusLEDStart(){
  pixels.begin();
  pixels.setBrightness(50);
  pixels.show(); // Initialize all pixels to 'off'
}

void SetStatus(){
    //strip.SetPixelColor(0, green);
    pixels.setPixelColor(0, pixels.Color(0, 150, 150));
    pixels.show();
}

char GetStatus(){
    return 0;
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
