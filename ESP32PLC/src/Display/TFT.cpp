
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
    // large block of text
    tft.fillScreen(TFT_BLACK);
    //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", TFT_WHITE);
    tft.setCursor(65, 12);
    tft.setTextColor(TFT_WHITE);
    tft.setTextWrap(true);
    tft.print(GetClientId().c_str());
    //delay(1000);
    TFTLog("Init.....");
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

char TFTLastWiFiSig = 0;

void TFTWiFiSignal(char overide){
  long Rssi = WiFi.RSSI()*-1;
  if(Rssi > HIGHRSSI){
    if((TFTLastWiFiSig != 1) || (overide == 1)){
      tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,SignalSmall, 16, 16, TFT_RED);
      TFTLastWiFiSig = 1;
    }
  }
  else if((Rssi < HIGHRSSI) && (Rssi > LOWRSSI)){
    if((TFTLastWiFiSig != 2) || (overide == 1)){
      tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,SignalMed, 16, 16, TFT_YELLOW);
      TFTLastWiFiSig = 2;
    }
  }
  else if(Rssi < LOWRSSI){
    if((TFTLastWiFiSig != 3)  || (overide == 1)){
      tft.drawBitmap(TFTWIFIBITMAPX,TFTWIFIBITMAPY,Sig, 16, 16, TFT_GREEN);
      TFTLastWiFiSig = 3;
    }
  }
}

void TFTTHBar(){
  TFTBarClear();
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(2, TFTBANNERY);
  tft.println("T: ");
  tft.setCursor(18, TFTBANNERY);
  tft.println(String(getDeviceClimateTemprature(),1));
  tft.setCursor(50, TFTBANNERY);
  tft.println("H: ");
  tft.setCursor(70, TFTBANNERY);
  tft.println(String(getDeviceClimateHumidity(),1));
}

void TFTBarClear(){
  tft.fillRect(0, TFTBANNERY, 200, 20, TFT_BLACK);
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
  //tft.drawBitmap(30,90,Logo, 180, 108, TFT_GREEN);
}

int16_t xpos = 60;
int16_t ypos = 60;

void pngDraw(PNGDRAW *pDraw){
  uint16_t lineBuffer[MAX_IMAGE_WDITH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void TFTLog(const char *Comment){
  tft.fillRect(INFOX, INFOY, 200, 20, TFT_BLACK);
  tft.setCursor(INFOX, INFOY);
  tft.setTextColor(TFT_WHITE);
  tft.setTextWrap(true);
  tft.print(Comment);
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