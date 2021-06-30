#include "Oled.h"
#include "OledBitMaps.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

char DisplayOK = 1;

void DisplayInit(void){
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    DisplayOK = 0;
  }
  if(DisplayOK){
    display.clearDisplay();
    display.display();
    delay(100);
  }
}

void MQTTIconSet(char IconMode){
  if(IconMode == 1){
    display.drawBitmap(MQTTCX,MQTTCY,Connected, 16, 16, 1);
    display.display();
  }
  else{
    Bitmap16xBitMapClear(2);
  } 
}

void WiFiStreanth(int power){
  if(DisplayOK == 1){
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

int p = 1;

void WiFiConnectAnimation(void){
  WiFiStreanth(p);
  p++;
  if(p>4){
    p = 1;
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
