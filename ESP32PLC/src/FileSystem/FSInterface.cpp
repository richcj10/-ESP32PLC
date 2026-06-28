#include "FSInterface.h"
#include "FileSystem.h"
#include "Devices/Log.h"
#include "Arduino.h"
#include <Preferences.h>
#include <LittleFS.h>
#include "WifiControl/WifiConfig.h"

WiFiConfig wfconfig;
MQTTConfig mqconfig;

char FileStstemStart() {
    return FileSystemInit(&wfconfig, &mqconfig);
}

// ── WiFi getters ────────────────────────────────────────────────────────────
unsigned char GetWiFiMode()     { return wfconfig.WIFIMode; }
String GetSSID()                { return String(wfconfig.SSID); }
String GetSSIDPassword()        { return String(wfconfig.Passcode); }
String GetHostName()            { return String(wfconfig.Host); }

// ── MQTT getters ────────────────────────────────────────────────────────────
bool     GetMQTTEnabled()       { return wfconfig.WIFIMode != 0 && mqconfig.MQTTEnabble != 0; }
String   GetMQTTIP()            { return String(mqconfig.MQTTIP); }
String   GetMQTTUser()          { return String(mqconfig.MQTTUser); }
String   GetMQTTPassword()      { return String(mqconfig.MQTTPassword); }
uint16_t GetMQTTPort()          { return mqconfig.MQTTPort; }

// ── Save WiFi config ─────────────────────────────────────────────────────────
bool SaveWiFiConfig(uint8_t mode, const char* ssid, const char* pass, const char* host) {
    wfconfig.WIFIMode = mode;
    strlcpy(wfconfig.SSID,     ssid ? ssid : "", sizeof(wfconfig.SSID));
    strlcpy(wfconfig.Passcode, pass ? pass : "", sizeof(wfconfig.Passcode));
    if (host && host[0])
        strlcpy(wfconfig.Host, host, sizeof(wfconfig.Host));
    wfconfig.SSIDLN  = strlen(wfconfig.SSID);
    wfconfig.PswdLN  = strlen(wfconfig.Passcode);
    wfconfig.HoastLN = strlen(wfconfig.Host);
    WifisaveConfiguration(&wfconfig);
    // Clear forced-AP flag whenever mode changes away from AP via any path.
    if (mode != WIFI_AP_MODE) LittleFS.remove("/forced_ap.flag");
    Log(LOG, "Config: WiFi saved (mode=%u ssid=%s)\r\n", mode, wfconfig.SSID);
    return true;
}

// ── Forced-AP flag (written by display switch page, persists across restart) ──
bool IsForcedAPMode() {
    return LittleFS.exists("/forced_ap.flag");
}

void SetForcedAPMode(bool forced) {
    if (forced) {
        File f = LittleFS.open("/forced_ap.flag", "w");
        if (f) f.close();
    } else {
        LittleFS.remove("/forced_ap.flag");
    }
}

// ── Remote config accessor ────────────────────────────────────────────────────
const RemoteConfig_t& GetRemoteConfig() {
    const RemoteConfig_t* p = RemoteGetConfig();
    static const RemoteConfig_t empty = {};  // safe fallback if called before alloc
    return p ? *p : empty;
}

// ── Debug / feature flags ─────────────────────────────────────────────────────
bool GetJoyCalPageEnabled() {
    Preferences p; p.begin("debug", true);
    bool v = p.getBool("joy_cal", false);
    p.end(); return v;
}
void SetJoyCalPageEnabled(bool en) {
    Preferences p; p.begin("debug", false);
    p.putBool("joy_cal", en);
    p.end();
}

// ── Factory reset ─────────────────────────────────────────────────────────────
void FactoryReset() {
    Preferences p;
    p.begin("wifi", false); p.clear(); p.end();
    p.begin("mqtt", false); p.clear(); p.end();
    LittleFS.remove("/Remote.json");
    Log(NOTIFY_FORCE, "FS: factory reset complete\r\n");
}

// ── Save MQTT config ──────────────────────────────────────────────────────────
bool SaveMQTTConfig(bool enabled, const char* ip, uint16_t port, const char* user, const char* pass) {
    mqconfig.MQTTEnabble = enabled ? 1 : 0;
    strlcpy(mqconfig.MQTTIP,       ip   ? ip   : "", sizeof(mqconfig.MQTTIP));
    strlcpy(mqconfig.MQTTUser,     user ? user : "esp32plc", sizeof(mqconfig.MQTTUser));
    strlcpy(mqconfig.MQTTPassword, pass ? pass : "", sizeof(mqconfig.MQTTPassword));
    mqconfig.MQTTPasswordLN = strlen(mqconfig.MQTTPassword);
    mqconfig.MQTTPort       = port ? port : 1883;
    MqttsaveConfiguration(&mqconfig);
    Log(LOG, "Config: MQTT saved (enabled=%u ip=%s port=%u)\r\n",
        mqconfig.MQTTEnabble, mqconfig.MQTTIP, mqconfig.MQTTPort);
    return true;
}
