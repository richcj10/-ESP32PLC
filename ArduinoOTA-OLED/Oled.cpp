#include "Oled.h"
#include "OledBitMaps.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Sensors.h"
#include "Functions.h" 

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

char DisplayOK = 1;

void DisplayInit(void){
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    DisplayOK = 0;
  }
  if(DisplayOK){
    Serial.println("Display OK");
    display.clearDisplay();
    //display.display();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(52, 2);
    display.println(GetClientId());
    display.drawBitmap(((display.width()-32)/2),((display.height()-32)/2),Home, 32, 32, 1);
    display.display();
    delay(1000);
  }
}

void MQTTIconSet(char IconMode){
  if(IconMode == 1){
    Bitmap16xBitMapClear(2);
    display.drawBitmap(MQTTCX,MQTTCY,Connected, 16, 16, 1);
    display.display();
  }
  else{
    Bitmap16xBitMapClear(2);
    display.drawBitmap(MQTTCX,MQTTCY,NotConnected, 16, 16, 1);
    display.display();
  } 
}

void WiFiStreanthDisplay(char power){
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
  WiFiStreanthDisplay(p);
  p++;
  if(p>4){
    p = 1;
  }
}

void WiFiAP(char Enable){
  if(Enable == 1){
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 2);
    display.println("AP");
    display.display(); 
  }
  else{
    Bitmap16xBitMapClear(2);
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

void DisplayTHBar(){
  THBarClear();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, BANNERY);
  display.println("T: ");
  display.setCursor(18, BANNERY);
  display.println(String(getDeviceClimateTemprature(),1));
  display.setCursor(50, BANNERY);
  display.println("H: ");
  display.setCursor(70, BANNERY);
  display.println(String(getDeviceClimateHumidity(),1));
  display.display();
}

void THBarClear(){
  for(int pixely=BANNERY;pixely<SCREEN_HEIGHT;pixely++)
  {
    for(int pixelx=0;pixelx<SCREEN_WIDTH;pixelx++){
      display.drawPixel(pixelx, pixely, 0);
    }
  }
}

void CenterClear(){
  for(int pixely=CENTERYTOP;pixely<CENTERYBOT;pixely++)
  {
    for(int pixelx=0;pixelx<SCREEN_WIDTH;pixelx++){
      display.drawPixel(pixelx, pixely, 0);
    }
  }
}

void FullDisplayClear(void){
    display.clearDisplay();
    display.display();
}

void DisplayCenterClear(void){
  display.fillRect(0, 20, display.width(), 48, 0);
  display.display();
}

void DisplayCenterChestTemp(void){
  CenterClear();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 20);
  display.println("Chest Temp: ");
  display.setCursor(38, 30);
  display.setTextSize(2);
  display.println(String(getOneWireTemprature(),1));
  display.display();
}
