
#include <TFT_eSPI.h> // Hardware-specific library
#include "TFT.h"
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

void TFTInit(){

    tft.init();   // initialize a ST7735S chip
    Serial.println("Initialized");
    
    uint16_t time = millis();
    tft.fillScreen(TFT_BLACK);
    time = millis() - time;

    Serial.println(time, DEC);
    delay(500);

    // large block of text
    tft.fillScreen(TFT_BLACK);
    //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", TFT_WHITE);
    tft.setCursor(20, 20);
    tft.setTextColor(TFT_RED);
    tft.setTextWrap(true);
    tft.print("Hi");
    delay(1000);
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}