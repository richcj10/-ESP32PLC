#include "FileSystem.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Define.h"
#include "Functions.h"
#include "FS.h"
#include <LittleFS.h>
#include "Devices/Log.h"

/* ── Uncomment to wipe LittleFS on next boot, then remove and reflash ──────── */
// #define FACTORY_RESET

static const char *WiFifilename  = "/WiFiconfig.json";
static const char *MQTTfilename  = "/MQTTconfig.json";
static const char *Remotefilename = "/Remote.json";

/* Only written, never read externally — keep internal */
static bool DefaultsLoaded = false;

static void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

// ----------------------------------------------------------------
// Init
// ----------------------------------------------------------------
char FileSystemInit(struct WiFiConfig* WFC, struct MQTTConfig* MQC) {
#ifdef FACTORY_RESET
    Log(NOTIFY, "FS: FACTORY RESET — formatting LittleFS\r\n");
    LittleFS.begin(true);
    LittleFS.format();
    LittleFS.end();
    Log(NOTIFY, "FS: format complete\r\n");
#endif

    if (!LittleFS.begin(false)) {
        Log(ERROR, "FS: LittleFS mount failed — formatting\r\n");
        if (!LittleFS.begin(true)) {
            Log(ERROR, "FS: LittleFS format+mount failed\r\n");
            return 0;
        }
        Log(NOTIFY, "FS: LittleFS formatted and mounted\r\n");
    }

    Log(LOG, "FS: mounted OK\r\n");
    listDir(LittleFS, "/", 0);

    WifiComfig(WFC);
    MqttComfig(MQC);
    RemoteComfig();
    return 1;
}

// ----------------------------------------------------------------
// WiFi config — load or create defaults (single open, no round-trip)
// ----------------------------------------------------------------
void WifiComfig(struct WiFiConfig* WFC) {
    if (!WifiloadConfiguration(WFC)) {
        Log(NOTIFY, "FS: no WiFi config — creating defaults (AP mode)\r\n");
        WifisaveConfiguration(WFC);   // auto-generates hostname, writes file
    } else if (WFC->HoastLN == 0 || strcmp(WFC->Host, "ESP32PLC") == 0) {
        /* Host is blank or still the generic default — generate unique name */
        strlcpy(WFC->Host, GetClientId().c_str(), sizeof(WFC->Host));
        WFC->HoastLN = strlen(WFC->Host);
        WifisaveConfiguration(WFC);   // persist the generated hostname
        Log(NOTIFY, "FS: hostname auto-generated: %s\r\n", WFC->Host);
    }
    PrintWiFiConfigStruct(WFC);
}

// ----------------------------------------------------------------
// MQTT config — load or create defaults (single open, no round-trip)
// ----------------------------------------------------------------
void MqttComfig(struct MQTTConfig* MQC) {
    if (!MqttloadConfiguration(MQC)) {
        Log(NOTIFY, "FS: no MQTT config — creating defaults (disabled)\r\n");
        MqttsaveConfiguration(MQC);
    }
    PrintMqttConfigStruct(MQC);
}

// ----------------------------------------------------------------
// Remote config — load and log (single open, smaller doc)
// ----------------------------------------------------------------
void RemoteComfig() {
    File f = LittleFS.open(Remotefilename);
    if (!f) { Log(NOTIFY, "FS: no Remote.json\r\n"); return; }
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) { Log(ERROR, "FS: Remote.json parse error: %s\r\n", err.c_str()); return; }
    Log(LOG, "FS: Remote.json OK (rev=%s)\r\n", doc["rev"] | "?");
}

// ----------------------------------------------------------------
// WiFi load — returns false if file does not exist
// Drops SSIDLN/PswdLN/HoastLN from JSON; derives them via strlen()
// ----------------------------------------------------------------
bool WifiloadConfiguration(struct WiFiConfig* WFC) {
    File file = LittleFS.open(WiFifilename);
    if (!file) return false;

    StaticJsonDocument<384> doc;          // reduced from 512; 8 fields, no derived
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Log(ERROR, "FS: WiFi config corrupt (%s) — rebuilding defaults\r\n", error.c_str());
        LittleFS.remove(WiFifilename);
        return false;
    }

    strlcpy(WFC->SSID,          doc["SSID"]          | "",              sizeof(WFC->SSID));
    strlcpy(WFC->Passcode,      doc["Passcode"]       | "",              sizeof(WFC->Passcode));
    strlcpy(WFC->Host,          doc["Host"]           | "ESP32PLC",      sizeof(WFC->Host));
    strlcpy(WFC->IP,            doc["IP"]             | "192.168.4.1",   sizeof(WFC->IP));
    strlcpy(WFC->DefultGateway, doc["DefultGateway"]  | "0.0.0.0",       sizeof(WFC->DefultGateway));
    strlcpy(WFC->SubMask,       doc["SubMask"]        | "255.255.255.0", sizeof(WFC->SubMask));

    WFC->WIFIMode = doc["WIFIMode"] | (unsigned char)2;
    WFC->DHCP     = doc["DHCP"]     | (unsigned char)1;
    WFC->SSIDLN   = strlen(WFC->SSID);       // derived — not stored in JSON
    WFC->PswdLN   = strlen(WFC->Passcode);
    WFC->HoastLN  = strlen(WFC->Host);

    Log(LOG, "FS: WiFi loaded (mode=%u ssid=%s)\r\n", WFC->WIFIMode, WFC->SSID);
    return true;
}

// ----------------------------------------------------------------
// WiFi save — no remove() before write; "w" truncates automatically
// Drops SSIDLN/PswdLN/HoastLN — derived fields don't belong in JSON
// ----------------------------------------------------------------
void WifisaveConfiguration(struct WiFiConfig* WFC) {
    DefaultsLoaded = true;

    strlcpy(WFC->Host, GetClientId().c_str(), sizeof(WFC->Host));
    WFC->HoastLN = strlen(WFC->Host);

    File file = LittleFS.open(WiFifilename, "w");   // "w" truncates existing file
    if (!file) { Log(ERROR, "FS: cannot open WiFi config for write\r\n"); return; }

    StaticJsonDocument<384> doc;
    doc["WIFIMode"]      = WFC->WIFIMode;
    doc["SSID"]          = WFC->SSID;
    doc["Passcode"]      = WFC->Passcode;
    doc["Host"]          = WFC->Host;
    doc["DHCP"]          = WFC->DHCP;
    doc["IP"]            = WFC->IP;
    doc["DefultGateway"] = WFC->DefultGateway;
    doc["SubMask"]       = WFC->SubMask;

    if (serializeJson(doc, file) == 0)
        Log(ERROR, "FS: WiFi config write failed\r\n");
    else
        Log(LOG, "FS: WiFi config saved\r\n");
    file.close();
}

// ----------------------------------------------------------------
// MQTT load — returns false if file does not exist
// Drops MQTTPasswordLN from JSON; derived via strlen()
// ----------------------------------------------------------------
bool MqttloadConfiguration(struct MQTTConfig* MQC) {
    File file = LittleFS.open(MQTTfilename);
    if (!file) return false;

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Log(ERROR, "FS: MQTT config corrupt (%s) — rebuilding defaults\r\n", error.c_str());
        LittleFS.remove(MQTTfilename);
        return false;
    }

    strlcpy(MQC->MQTTIP,       doc["MQTTIP"]       | "",          sizeof(MQC->MQTTIP));
    strlcpy(MQC->MQTTUser,     doc["MQTTUser"]     | "esp32plc",  sizeof(MQC->MQTTUser));
    strlcpy(MQC->MQTTPassword, doc["MQTTPassword"] | "",          sizeof(MQC->MQTTPassword));
    MQC->MQTTEnabble    = doc["MQTTEnabble"] | (unsigned char)0;
    MQC->MQTTPort       = doc["MQTTPort"]    | (uint16_t)1883;
    MQC->MQTTPasswordLN = strlen(MQC->MQTTPassword);  // derived

    Log(LOG, "FS: MQTT loaded (enabled=%u ip=%s port=%u)\r\n",
        MQC->MQTTEnabble, MQC->MQTTIP, MQC->MQTTPort);
    return true;
}

// ----------------------------------------------------------------
// MQTT save — no remove() before write
// Drops MQTTPasswordLN — derived field doesn't belong in JSON
// ----------------------------------------------------------------
void MqttsaveConfiguration(struct MQTTConfig* MQC) {
    File file = LittleFS.open(MQTTfilename, "w");   // "w" truncates existing file
    if (!file) { Log(ERROR, "FS: cannot open MQTT config for write\r\n"); return; }

    StaticJsonDocument<256> doc;
    doc["MQTTEnabble"]  = MQC->MQTTEnabble;
    doc["MQTTIP"]       = MQC->MQTTIP;
    doc["MQTTUser"]     = MQC->MQTTUser;
    doc["MQTTPassword"] = MQC->MQTTPassword;
    doc["MQTTPort"]     = MQC->MQTTPort;

    if (serializeJson(doc, file) == 0)
        Log(ERROR, "FS: MQTT config write failed\r\n");
    else
        Log(LOG, "FS: MQTT config saved\r\n");
    file.close();
}

// ----------------------------------------------------------------
// Debug print helpers
// ----------------------------------------------------------------
void PrintWiFiConfigStruct(struct WiFiConfig* WFC) {
    Log(LOG, "WiFi | mode=%u ssid=%s host=%s dhcp=%u ip=%s\r\n",
        WFC->WIFIMode, WFC->SSID, WFC->Host, WFC->DHCP, WFC->IP);
}

void PrintMqttConfigStruct(struct MQTTConfig* MQC) {
    Log(LOG, "MQTT | enabled=%u ip=%s port=%u user=%s\r\n",
        MQC->MQTTEnabble, MQC->MQTTIP, MQC->MQTTPort, MQC->MQTTUser);
}

// ----------------------------------------------------------------
// Directory listing — DEBUG level only, startup diagnostic
// ----------------------------------------------------------------
static void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) return;
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Log(DEBUG, "FS  DIR : %s\r\n", file.name());
            if (levels) listDir(fs, file.name(), levels - 1);
        } else {
            Log(DEBUG, "FS  FILE: %s  %u B\r\n", file.name(), (unsigned)file.size());
        }
        file = root.openNextFile();
    }
}
