#include "Functions.h"
#include <LittleFS.h>
#include "Sensors.h"
#include "MQTT.h"
#include "Display/Display.h"
#include "HAL/Digital/Digital.h"
#include <WiFi.h>
#include "HAL/Com/I2C.h"
#include "Devices/StatusLED.h"
#include "Devices/JoyStick.h"
#include "Webportal.h"
#include "Remote/MasterController.h"
#include "Remote/FwUpdater.h"
#include "Devices/Log.h"
#include "Display/TFT.h"
#include "WifiControl/WifiConfig.h"
#include "FileSystem/FSInterface.h"
#include "Devices/LEDStrip.h"
#include "Modules/ModuleManager.h"
#include "Devices/BootReset.h"

unsigned long lastUpdate = 0;
unsigned long lastSensorScanRate = 0;
String clientId = "";
int SensorScanRate = 1000;
char WiFiConnected = 0;

void SystemStart(){
  StatusLEDStart();
  Serial.setTxBufferSize(2048);   // log writes queue instead of blocking the loop
  Serial.begin(115200);
  LogSetup(GetSavedLogLevel(), 1);   // level set on the web Log tab, kept in NVS
  LogRingInit();
  BootResetStart();   // watches the button for 10 s in the background — see BootReset.cpp
  ClientIdCreation();
  Log(DEBUG,"LED Start-");
  LEDBoot();
  Log(DEBUG,"TFT Start-");
  DispalyConfigSet(TFT);
  DisplaySetup();
  DisplayBrightnes(75);
  Log(DEBUG,">> FileStstemStart");
  DisplayLog(FileStstemStart() ? "FS: OK" : "FS: ERROR");
  Log(DEBUG,"RS485 Master Start");
  RemoteStart();
  {
    const RemoteConfig_t& rc = GetRemoteConfig();
    uint8_t grps = RemoteGrpCount();
    char rmsg[36];
    if (rc.loaded)
      snprintf(rmsg, sizeof(rmsg), "RS485: %udev %ugrp %s",
               (unsigned)rc.deviceCount, (unsigned)grps,
               grps > 0 ? "OK" : "WARN");
    else
      snprintf(rmsg, sizeof(rmsg), "RS485: no cfg (default)");
    DisplayLog(rmsg);
  }
  Log(DEBUG,"I2C Master Start");
  I2CStart();
  {
    uint8_t addrs[8];
    uint8_t n = I2CScanAddrs(addrs, 8);
    if (n == 0) {
      DisplayLog("I2C: no devices");
    } else {
      for (uint8_t i = 0; i < n; i++) {
        char imsg[36];
        snprintf(imsg, sizeof(imsg), "I2C: %s", I2CDeviceName(addrs[i]));
        DisplayLog(imsg);
      }
    }
  }
  Log(DEBUG,"Joystick Start");
  JoyStickStart();
  Log(DEBUG,"Module Start");
  modules.begin();
  for (uint8_t i = 0; i < modules.count(); i++) {
    char mmsg[32];
    snprintf(mmsg, sizeof(mmsg), "Mod: %s%s", modules.get(i)->name(),
             modules.isEnabled(i) ? "" : " (off)");
    DisplayLog(mmsg);
  }
  if (modules.count() == 0)
    DisplayLog("Mod: none");
}

static bool         _fwDoneShown = false;
static unsigned long _fwDoneTime  = 0;

void UIUpdateLoop(){
  JoyStickUpdate();
  LEDUpdate();

  const FwUpdater::Status& fws = fwUpdater.getStatus();

  if (fws.running) {
    /* Device FW flash in progress — take over the display */
    _fwDoneShown = false;
    char msg[32];
    snprintf(msg, sizeof(msg), "Slave 0x%02X", (unsigned)fws.slaveId);
    DisplayUploadStatus("DEVICE UPDATE", fws.progress, msg);
    WebHandel();
    return;
  }

  if (fws.done && !_fwDoneShown) {
    /* Show result screen once, hold it for 3 s */
    _fwDoneShown = true;
    _fwDoneTime  = millis();
    DisplayUploadDone(fws.success, fws.message);
  }

  if (fws.done && (millis() - _fwDoneTime < 3000)) {
    WebHandel();
    return;
  }

  DisplayManager();
  WebHandel();
  modules.update();
}

void SensorUpdateLoop(){
  if (millis() - lastSensorScanRate >= SensorScanRate) {
    // save the last time you blinked the LED
    lastSensorScanRate = millis();
    UpdateSensors();
  }
}

void SyncLoop(){
  ArduinoOTA.handle();
  unsigned long now = millis();
  if (now - lastUpdate > 700){
    lastUpdate = now;
    ToggletUserLED();
    if(GetWiFisetupMode() != 2){
      if ((WiFi.status() != WL_CONNECTED)) {
        Serial.println("Rebooting - Lost WiFi");
        delay(1000);
        ESP.restart();
      }
    }
  }
}

void setup_ota() {
  ArduinoOTA.setHostname(GetClientId().c_str());
  //ArduinoOTA.setPassword(WiFiSettings.password.c_str());

  ArduinoOTA.onStart([]() {
    bool isFS = (ArduinoOTA.getCommand() == U_SPIFFS);
    if (isFS) {
      LittleFS.end();
      Log(NOTIFY_FORCE, "OTA FS Update!");
    } else {
      Log(NOTIFY_FORCE, "OTA FW Update!");
    }
    DisplayUploadStatus(isFS ? "OTA FS" : "OTA UPDATE", 0, "Starting...");
  });
  ArduinoOTA.onEnd([]() {
    Log(NOTIFY_FORCE, "OTA Update - Complete");
    DisplayUploadDone(true, "Restarting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    bool    isFS = (ArduinoOTA.getCommand() == U_SPIFFS);
    uint8_t pct  = (total > 0) ? (uint8_t)((float)progress / (float)total * 100.0f) : 0;
    static uint8_t lastPct = 0xFF;
    if (pct != lastPct) {
        lastPct = pct;
        DisplayUploadStatus(isFS ? "OTA FS" : "OTA UPDATE", pct, "Uploading...");
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    const char* errMsg = (error == OTA_AUTH_ERROR)    ? "Auth Failed"    :
                         (error == OTA_BEGIN_ERROR)   ? "Begin Failed"   :
                         (error == OTA_CONNECT_ERROR) ? "Connect Failed" :
                         (error == OTA_RECEIVE_ERROR) ? "Receive Failed" :
                         (error == OTA_END_ERROR)     ? "End Failed"     : "Unknown Error";
    Log(NOTIFY_FORCE, "OTA Error: %s", errMsg);
    DisplayUploadDone(false, errMsg);
    if (error != OTA_AUTH_ERROR) ESP.restart();
  });

  ArduinoOTA.begin();
}

void WiFiOK(){
  WiFiConnected = 1;
  modules.startNetwork();
}

void LEDWebSetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
  ledStrip.setColor(r, g, b, bri);
}
void LEDWebSetOff() {
  ledStrip.setOff();
}

char GetWiFiStatus(void){
  return(WiFiConnected);
}

String GetClientId(void){
  return(clientId);
}

void ClientIdCreation(void){
  byte mac[6];
  WiFi.macAddress(mac);
  clientId = "ESPPLC-";
  clientId = clientId + String(mac[4]) + String(mac[5]);
  Log(NOTIFY_FORCE,"ClientID = %s",clientId.c_str());
}
