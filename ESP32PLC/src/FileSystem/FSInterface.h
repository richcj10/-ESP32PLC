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

// Factory reset — clears NVS wifi+mqtt namespaces and removes Remote.json
void FactoryReset();

#endif
