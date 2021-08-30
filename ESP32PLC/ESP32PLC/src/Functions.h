#ifndef FUNCTIONS_H
#define  FUNCTIONS_H

#include "Define.h"
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
char GetWiFiStatus(void);
void WiFiFaiure(void);
void SaveResetReason(void);
char GetResetReason(char cpucore);
void PrintResetReason(void);

#endif  /* FUNCTIONS_H */
