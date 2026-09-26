#ifndef FUNCTIONS_H
#define  FUNCTIONS_H

#include <Arduino.h>
#include "Define.h"
#include <ArduinoOTA.h>
#include <Wire.h>

void setup_ota(void);
String GetClientId(void);
void SystemStart(void);
void ClientIdCreation(void);
void SyncLoop(void);
// Main-loop timing since the last call (resets the window). Defined in main.cpp.
void LoopStatsRead(uint32_t* maxUs, uint32_t* peakUs, uint32_t* avgUs, uint32_t* perSec);
char GetWiFiStatus(void);
void UIUpdateLoop();
void SensorUpdateLoop();
void WiFiOK();

void LEDWebSetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t bri);
void LEDWebSetOff();

#endif  /* FUNCTIONS_H */
