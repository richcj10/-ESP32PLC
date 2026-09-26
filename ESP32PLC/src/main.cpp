#include "Functions.h"
#include "MQTT.h"
#include "Sensors.h"

#include "Display/Display.h"
#include "Display/UIPages.h"
#include "HAL/DeviceConfig.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "HAL/Digital/Digital.h"
#include "HAL/Com/I2C.h"
#include "Devices/StatusLED.h"
#include "Devices/JoyStick.h"
#include "Webportal.h"
#include "Devices/Log.h"
#include "Remote/MasterController.h"
#include "Remote/FwUpdater.h"
#include "Display/TFT.h"
#include "Modules/ModuleManager.h"

void setup() {
  SystemStart();
   delay(1);
  Log(DEBUG,">> SystemStart done");
  pinMode(16, INPUT);
  LEDBoot();
  Log(DEBUG,">> QueryLocalDevice");
  QueryLocalDevice();
  DisplayLog(GetWiFiMode() != 0 ? "NVS: OK" : "NVS: defaults");
  Log(DEBUG,">> DigitalStart");
  DigitalStart();
  Log(DEBUG,">> IOStart");
  IOStart();
  DisplayLog("WiFi: Connecting...");
  Log(DEBUG,">> SetupWiFi");
  SetupWiFi();   // retries STA 3x then falls back to AP internally
  Log(DEBUG,">> setup_ota");
  setup_ota();
  if (GetWiFiMode() == WIFI_AP_MODE) {
    DisplayLog("WiFi: AP Mode");
  } else {
    char _wip[48];
    snprintf(_wip, sizeof(_wip), "WiFi: %s", GetIPStr().c_str());
    DisplayLog(_wip);
  }
  delay(1);
  Log(DEBUG,">> InitSensors");
  InitSensors();
  DisplayLog(GetMQTTEnabled() ? "MQTT: Enabled" : "MQTT: Disabled");
  Log(DEBUG,">> MQTTStart");
  MQTTStart();
  Log(DEBUG,">> WebStart");
  WebStart();
  Log(DEBUG,">> fwUpdater.init");
  fwUpdater.init();
  Log(DEBUG,">> SetLEDStatus");
  SetLEDStatus(NORMAL,1000);
  Serial.println("Setup Done!");
  Log(DEBUG,">> DisplayTimeoutReset");
  DisplayTimeoutReset();
  Log(DEBUG,">> WiFiMode check");
  if (GetWiFiMode() == WIFI_AP_MODE) {
    String apSsid = GetSanitizedHostname();
    DisplaySetAPMode(true, apSsid.c_str());
    DisplayAPInfo(apSsid.c_str());
  } else {
    DisplaySetAPMode(false, nullptr);
    DisplayClear();
    Log(DEBUG,">> UIPageInit");
    UIPageInit();
    Log(DEBUG,">> UIPageDraw");
    UIPageDraw();
  }
  Log(DEBUG,">> Setup complete");
}

static unsigned long _lastRemotePub = 0;

// ── Loop timing (shown on the web Log tab via /api/mem) ─────────────────────
// Max is since the last read (reset by LoopStatsRead) plus an all-time peak.
static uint32_t _loopMaxUs = 0, _loopPeakUs = 0;
static uint64_t _loopSumUs = 0;
static uint32_t _loopCount = 0;
static uint32_t _loopWinStartMs = 0;

void LoopStatsRead(uint32_t* maxUs, uint32_t* peakUs, uint32_t* avgUs, uint32_t* perSec) {
  uint32_t winMs = millis() - _loopWinStartMs;
  *maxUs  = _loopMaxUs;
  *peakUs = _loopPeakUs;
  *avgUs  = _loopCount ? (uint32_t)(_loopSumUs / _loopCount) : 0;
  *perSec = winMs ? (uint32_t)((uint64_t)_loopCount * 1000 / winMs) : 0;
  _loopMaxUs = 0; _loopSumUs = 0; _loopCount = 0; _loopWinStartMs = millis();
}

void loop() {
  uint32_t _t0 = micros();
  WiFiRecoveryLoop();
  CaptivePortalLoop();
  ScanIO();
  RemoteRun();
  UIUpdateLoop();
  SensorUpdateLoop();
  MqttLoop();
  SyncLoop();
  SendLocalIO();   // publishes only on change (+ hourly heartbeat) — cheap every loop

  if (millis() - _lastRemotePub > 1500) {
    _lastRemotePub = millis();
    SendRemoteDevices();
    modules.mqttPublish();
  }

  // Loop work time — measured before delay(1) so the idle yield isn't counted
  uint32_t _dt = micros() - _t0;
  if (_dt > _loopMaxUs)  _loopMaxUs  = _dt;
  if (_dt > _loopPeakUs) _loopPeakUs = _dt;
  _loopSumUs += _dt; _loopCount++;
  delay(1);
}
