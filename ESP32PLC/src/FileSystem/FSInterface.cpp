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
    // Blank password = keep the stored one. The web page never sends the saved
    // password back to the browser, so a save with the field left blank must NOT
    // wipe it. Pass a real password only to change it.
    if (pass && pass[0])
        strlcpy(wfconfig.Passcode, pass, sizeof(wfconfig.Passcode));
    if (host && host[0])
        strlcpy(wfconfig.Host, host, sizeof(wfconfig.Host));
    wfconfig.SSIDLN  = strlen(wfconfig.SSID);
    wfconfig.PswdLN  = strlen(wfconfig.Passcode);
    wfconfig.HoastLN = strlen(wfconfig.Host);
    WifisaveConfiguration(&wfconfig);
    // User configured WiFi from the web — the open AP after a button reset is no longer needed
    if (IsAPOpen()) SetAPOpen(false);
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
// RemoteGetConfig() allocates on first use and never returns null. (A `static const
// RemoteConfig_t empty = {}` fallback used to live here — it put ~270 KB of zeros
// into flash.)
const RemoteConfig_t& GetRemoteConfig() {
    return *RemoteGetConfig();
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

// Log level (1=ERROR .. 4=DEBUG) — read once at boot, written when changed on the web
uint8_t GetSavedLogLevel() {
    Preferences p; p.begin("debug", true);
    uint8_t v = p.getUChar("log_lvl", LOG);
    p.end(); return v;
}
void SetSavedLogLevel(uint8_t level) {
    Preferences p; p.begin("debug", false);
    p.putUChar("log_lvl", level);
    p.end();
}

// ── Device label ──────────────────────────────────────────────────────────────
// Stored in NVS so a filesystem upload does not wipe it.
// Falls back to legacy /label.txt once, then migrates.
String GetDeviceLabel() {
    Preferences p; p.begin("device", true);
    bool found = p.isKey("label");
    String v = found ? p.getString("label", "") : String();
    p.end();
    if (found) return v;

    if (LittleFS.exists("/label.txt")) {
        File f = LittleFS.open("/label.txt", "r");
        if (f) { v = f.readString(); f.close(); }
        SetDeviceLabel(v.c_str());
        LittleFS.remove("/label.txt");
        Log(LOG, "NVS: device label migrated from label.txt\r\n");
    }
    return v;
}

void SetDeviceLabel(const char* label) {
    char buf[65];
    strlcpy(buf, label ? label : "", sizeof(buf));
    Preferences p; p.begin("device", false);
    p.putString("label", buf);
    p.end();
}

// ── Factory reset ─────────────────────────────────────────────────────────────
void NetworkReset(bool openAP) {
    Preferences p;
    p.begin("wifi", false); p.clear(); p.end();
    p.begin("mqtt", false); p.clear(); p.end();
    // Legacy JSON configs would be migrated back into NVS on next boot — remove them
    LittleFS.remove("/WiFiconfig.json");
    LittleFS.remove("/MQTTconfig.json");
    SetAPOpen(openAP);
    Log(NOTIFY_FORCE, "FS: network reset complete (AP %s)\r\n", openAP ? "open" : "with password");
}

void FactoryReset(bool openAP) {
    NetworkReset(openAP);
    Preferences p;
    p.begin("device", false); p.clear(); p.end();
    LittleFS.remove("/Remote.json");
    LittleFS.remove("/label.txt");
    Log(NOTIFY_FORCE, "FS: factory reset complete\r\n");
}

// ── Open-AP flag ──────────────────────────────────────────────────────────────
static int8_t _apOpen = -1;   // -1 = not read yet

bool IsAPOpen() {
    if (_apOpen < 0) {
        Preferences p; p.begin("apcfg", true);
        _apOpen = p.getBool("open", false) ? 1 : 0;
        p.end();
    }
    return _apOpen == 1;
}

void SetAPOpen(bool open) {
    Preferences p; p.begin("apcfg", false);
    if (open) p.putBool("open", true); else p.remove("open");
    p.end();
    _apOpen = open ? 1 : 0;
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
