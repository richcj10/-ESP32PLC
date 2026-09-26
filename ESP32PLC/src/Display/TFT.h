#ifndef TFT_H
#define  TFT_H

#include <TFT_eSPI.h>

// Full-screen PSRAM back-buffer — draw here, push once with Screen.pushSprite(0,0)
extern TFT_eSprite Screen;

void TFTInit();
void TFTDisplayClear();
void TFTLogo();

void TFTDisplayAPInfo(const char* ssid);
void TFTBootLog(const char* line);

#endif