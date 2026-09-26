#ifndef FSINTERFACE_H
#define FSINTERFACE_H
#include "Arduino.h"
#include "FileSystem.h"

char FileStstemStart();

// WiFi config getters
unsigned char GetWiFiMode();
String GetSSID();
String GetSSIDPassword();
String GetHostName();

// MQTT config getters
bool     GetMQTTEnabled();
String   GetMQTTIP();
String   GetMQTTUser();
String   GetMQTTPassword();
uint16_t GetMQTTPort();

// Config save helpers (validate, update struct, write to LittleFS)
bool SaveWiFiConfig(uint8_t mode, const char* ssid, const char* pass, const char* host);
bool SaveMQTTConfig(bool enabled, const char* ip, uint16_t port, const char* user, const char* pass);

// Remote device config accessor
const RemoteConfig_t& GetRemoteConfig();

// Config file revision check
bool        RemoteConfigRevOK();      // false if loaded rev != firmware's expected rev
const char* RemoteConfigRevGot();     // rev string found in the file
const char* RemoteConfigRevNeeded();  // rev string this firmware requires

// Forced-AP flag — set by display switch page, cleared when switching back to STA
bool IsForcedAPMode();
void SetForcedAPMode(bool forced);

// Factory reset — clears NVS wifi+mqtt+device namespaces and removes Remote.json
// openAP=true: next AP start is an open network (no password / no QR) — used by
// the boot-button reset so the device can be reached without a working LCD.
void FactoryReset(bool openAP = false);
// Network reset — clears WiFi + MQTT only (keeps Remote.json, label, apps)
void NetworkReset(bool openAP = false);

// Open-AP flag (NVS "apcfg"). Cleared when WiFi settings are saved from the web.
bool IsAPOpen();          // cached — cheap to call
void SetAPOpen(bool open);

// Device label / name shown in the web UI (NVS namespace "device", max 64 chars)
String GetDeviceLabel();
void   SetDeviceLabel(const char* label);

// Debug / feature flags (NVS namespace "debug")
bool GetJoyCalPageEnabled();
void SetJoyCalPageEnabled(bool en);
uint8_t GetSavedLogLevel();              // 1=ERROR 2=LOG 3=NOTIFY 4=DEBUG, default LOG
void    SetSavedLogLevel(uint8_t level);

#endif
