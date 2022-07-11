
#include "TFT.h"
#include <TFT_eSPI.h> // Hardware-specific library
#include <Arduino.h>
#include <SPI.h>
#include "OledBitMaps.h"
#include "Functions.h"
#include <PNGdec.h>
#include <LittleFS.h>
#include <FS.h>
#include <time.h>
#include "Sensors.h"
#include "TFTBitMaps.h"

PNG png;

#define MAX_IMAGE_WDITH 240 // Adjust for your images
//FILE pngfile;

// void pngDraw(PNGDRAW *pDraw);
// void pngDraw(PNGDRAW *pDraw);
// void * pngOpen(const char *filename, int32_t *size);
// void pngClose(void *handle);
// int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length);
// int32_t pngSeek(PNGFILE *page, int32_t position);

TFT_eSPI tft = TFT_eSPI(320,240);       // Invoke custom library
void pngDraw(PNGDRAW *pDraw);

void TFTInit(){

    tft.init();   // initialize a ST7735S chip
    Serial.println("TFT-Config");
    tft.invertDisplay(1);
    tft.fillScreen(TFT_BLACK);
    TFTLog("Init.....");
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

char TFTLastWiFiSig = 0;

void TFTTHBar(){
  TFTBarClear();
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(25, TFTBANNERY);
  tft.println("T: ");
  tft.setCursor(55, TFTBANNERY);
  tft.println(String(getDeviceClimateTemprature(),1));
  tft.setCursor(110, TFTBANNERY);
  tft.println("H: ");
  tft.setCursor(140, TFTBANNERY);
  tft.println(String(getDeviceClimateHumidity(),1));
}

void TFTIDSet(){
  tft.setTextSize(1);
  tft.setCursor(100, 8);
  tft.setTextColor(TFT_WHITE);
  tft.print(GetClientId().c_str());
}

void TFTMQTTIconSet(char IconMode){
  if(IconMode == 1){
    tft.drawBitmap(TFTMQTTCX,TFTMQTTCY,Clear, 16, 16, TFT_BLACK);
    tft.drawBitmap(TFTMQTTCX,TFTMQTTCY,Connected, 16, 16, TFT_WHITE);
  }
  else{
    tft.drawBitmap(TFTMQTTCX,TFTMQTTCY,Clear, 16, 16, TFT_BLACK);
    tft.drawBitmap(TFTMQTTCX,TFTMQTTCY,NotConnected, 16, 16, TFT_WHITE);
  } 
}

void TFTWiFiConnect(char Position){
  if(Position == 1){
    tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,Clear, 16, 16, TFT_BLACK);
    tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,SignalSmall, 16, 16, TFT_RED);
  }
  if(Position == 2){
    tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,Clear, 16, 16, TFT_BLACK);
    tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,SignalMed, 16, 16, TFT_YELLOW);
  }
  if(Position == 3){
    tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,Clear, 16, 16, TFT_BLACK);
    tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,Sig, 16, 16, TFT_GREEN);
  }
}

void TFTDisplayInputs(void){
  TFTCenterClear();
  tft.setTextSize(2);
  tft.setCursor(30, 20);
  tft.print("Inputs:");
  tft.drawCircle(30, 60, 4, TFT_WHITE);
  tft.drawCircle(50, 60, 4, TFT_WHITE);
  tft.drawCircle(70, 60, 4, TFT_WHITE);
  tft.drawCircle(90, 60, 4, TFT_WHITE);
}

void TFTDisplayOutputs(void){
  TFTCenterClear();
  tft.setTextSize(2);
  tft.setCursor(30, 20);
  tft.print("Outputs:");
  tft.drawCircle(30, 60, 4, TFT_WHITE);
  tft.drawCircle(50, 60, 4, TFT_WHITE);
  tft.drawCircle(70, 60, 4, TFT_WHITE);
  tft.drawCircle(90, 60, 4, TFT_WHITE);
}

void TFTDisplayRemote(void){
  TFTCenterClear();
  tft.setTextSize(2);
  tft.setCursor(60, 50);
  tft.setTextColor(TFT_WHITE);
  tft.print("Remote:");
}

void TFTDisplayIP(){
  TFTCenterClear();
  tft.setTextSize(2);
  tft.setCursor(60, 50);
  tft.setTextColor(TFT_WHITE);
  tft.print("IP: 0.0.0.0");
  tft.setCursor(60, 70);
  tft.print("MQTT: 0.0.0.0");
}

void TFTLogo(){
  int16_t rc = png.openFLASH((uint8_t *)Logo, sizeof(Logo), pngDraw);
  Serial.println("png file try");
  if (rc == PNG_SUCCESS) {
    Serial.println("Successfully png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt); Serial.println("ms");
    tft.endWrite();
    // png.close(); // not needed for memory->memory decode
  }
}

int16_t xpos = 40;
int16_t ypos = 60;

void pngDraw(PNGDRAW *pDraw){
  uint16_t lineBuffer[MAX_IMAGE_WDITH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void TFTLog(const char *Comment){
  tft.fillRect(INFOX, INFOY, 200, 15, TFT_BLACK);
  tft.setCursor(INFOX+5, INFOY);
  tft.setTextColor(TFT_WHITE);
  tft.setTextWrap(true);
  tft.print(Comment);
}

void TFTDisplayClear(){
  tft.fillScreen(TFT_BLACK);
}

void TFTBarClear(){
  tft.fillRect(0, TFTBANNERY, 200, 20, TFT_BLACK);
}

void TFTCenterClear(){
  tft.fillRect(0, 20, 200, 120, TFT_BLACK);
}


// void Bitmap16xBitMapClear(char Location){
//   char x,y = 0;
//   if(Location == 1){
//     x = TFTWIFIBITMAPX;
//     y = TFTWIFIBITMAPY;
//   }
//   if(Location == 2){
//     x = MQTTCX;
//     y = MQTTCY;
//   }
//   tft.drawBitmap(x,y,Clear, 16, 16, TFT_BLACK);
// }
#define MAX_IMAGE_WDITH 240

// void TFTLogoDisplay(){
//   int16_t rc = png.open("/Logo.png", pngOpen, pngClose, pngRead, pngSeek, pngDraw);
//   if (rc == PNG_SUCCESS) {
//     tft.startWrite();
//     Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
//     uint32_t dt = millis();
//     if (png.getWidth() > MAX_IMAGE_WDITH) {
//       Serial.println("Image too wide for allocated line buffer size!");
//     }
//     else {
//       rc = png.decode(NULL, 0);
//       png.close();
//     }
//     tft.endWrite();
//     // How long did rendering take...
//     Serial.print(millis()-dt); Serial.println("ms");
//   }
// }

// void pngDraw(PNGDRAW *pDraw) {
//   uint16_t lineBuffer[MAX_IMAGE_WDITH];
//   png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
//   tft.pushImage(50, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer);
// }

// void * pngOpen(const char *filename, int32_t *size) {
//   Serial.printf("Attempting to open %s\n", filename);
//   pngfile = LittleFS.open(filename, "r");
//   *size = pngfile.size();
//   return &pngfile;
// }

// void pngClose(void *handle) {
//   pngfile = *((File*)handle);
//   if (pngfile) pngfile.close();
// }

// int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length) {
//   if (!pngfile) return 0;
//   page = page; // Avoid warning
//   return pngfile.read(buffer, length);
// }

// int32_t pngSeek(PNGFILE *page, int32_t position) {
//   if (!pngfile) return 0;
//   page = page; // Avoid warning
//   return pngfile.seek(position);
// }