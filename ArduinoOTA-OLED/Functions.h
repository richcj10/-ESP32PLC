#ifndef FUNCTIONS_H
#define  FUNCTIONS_H

#include "Define.h"
#include <SPIFFS.h>
#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <Wire.h>


void setup_ota(void);
String GetClientId(void);
void SystemStart(void);
void GPIOInit(void);
void SystemStart(void);
void ClientIdCreation(void);
void WiFiStart(void);
void SyncLoop(void);
char GetWiFiStatus();
void WiFiFaiure();

#endif  /* FUNCTIONS_H */
