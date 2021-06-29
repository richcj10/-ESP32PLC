#include "Oled.h"
#include "OledBitMaps.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

char DisplayOK = 0;

void DisplayInit(void){
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    DisplayOK = 1;
  }
  if(DisplayOK){
    display.clearDisplay();
    display.display();
  }
}

void WiFiStreanth(char power){
  if(DisplayOK){
    Bitmap16xBitMapClear(1);
    if(power == 0){
      display.drawBitmap(WIFIBITMAPX,WIFIBITMAPY,NotConnected, 16, 16, 1);
    }
    if(power == 1){
      display.drawBitmap(WIFIBITMAPX,WIFIBITMAPY,SignalSmall, 16, 16, 1);
    }
    else if(power == 2){
      display.drawBitmap(WIFIBITMAPX,WIFIBITMAPY,SignalMed, 16, 16, 1);
    }
    else if(power == 3){
      display.drawBitmap(WIFIBITMAPX,WIFIBITMAPY,Sig, 16, 16, 1);
    }
    else{
      Bitmap16xBitMapClear(1);
    }
    display.display();
  }
}

void WiFiConnectAnimation(char DelayTime){
  for(char p=1;p<3;p++){
    WiFiStreanth(p);
    delay(DelayTime);
  }
}

void Bitmap16xBitMapClear(char Location){
  char x,y = 0;
  if(DisplayOK){
    if(Location == 1){
        x = WIFIBITMAPX;
        y = WIFIBITMAPY;
    }
    if(Location == 2){
        x = MQTTCX;
        y = MQTTCY;
    }
    display.drawBitmap(x,y,Clear, 16, 16, 0);
    display.display();
  }
}
