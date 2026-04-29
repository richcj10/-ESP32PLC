#ifndef FUNCTIONS_H
#define  FUNCTIONS_H

#include <Arduino.h>
#include "Define.h"
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
void UIUpdateLoop();
void SensorUpdateLoop();
void WiFiOK();

void SetFWData(char ch1,char ch2,char ch3,char ch4,char ch5,int ch1t, int ch2t,int ch3t,int ch4t,int ch5t);
void RunFW();
void FireWorksTrigger(char CH);
char GetCHFire(char ch);
void SetCHFire(char ch, char data);

#endif  /* FUNCTIONS_H */
