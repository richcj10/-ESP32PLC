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
// Remote device config — static storage + load / save
// ----------------------------------------------------------------
static RemoteConfig_t _remoteCfg;

const RemoteConfig_t* RemoteGetConfig() { return &_remoteCfg; }

void RemoteComfig() {
    memset(&_remoteCfg, 0, sizeof(_remoteCfg));

    File f = LittleFS.open(Remotefilename);
    if (!f) { Log(NOTIFY, "FS: no Remote.json\r\n"); return; }

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) { Log(ERROR, "FS: Remote.json parse error: %s\r\n", err.c_str()); return; }

    JsonArray devArr = doc["devices"].as<JsonArray>();
    if (devArr.isNull()) { Log(NOTIFY, "FS: Remote.json — no devices array\r\n"); return; }

    for (JsonObject dev : devArr) {
        if (_remoteCfg.deviceCount >= MAX_REMOTE_DEVICES) break;
        RemoteDeviceCfg_t& d = _remoteCfg.devices[_remoteCfg.deviceCount];

        strlcpy(d.name,       dev["name"]          | "Unknown", sizeof(d.name));
        d.address         = dev["address"]       | (uint8_t)1;
        d.typeId          = dev["typeId"]        | (uint8_t)0;
        d.swVersion       = dev["swVersion"]     | (uint16_t)0;
        d.versionRegIdx   = dev["versionRegIdx"] | (uint8_t)0;
        d.versionRegType  = dev["versionRegType"]| (uint8_t)0;

        JsonArray grpArr = dev["groups"].as<JsonArray>();
        for (JsonObject grp : grpArr) {
            if (d.groupCount >= MAX_GROUPS_PER_DEVICE) break;
            RemoteGroupCfg_t& g = d.groups[d.groupCount];

            strlcpy(g.name,      grp["name"]       | "Group", sizeof(g.name));
            g.fc         = grp["fc"]         | (uint8_t)4;
            g.startReg   = grp["startReg"]   | (uint16_t)0;
            g.count      = grp["count"]      | (uint8_t)1;
            g.pollMs     = grp["pollMs"]     | (uint32_t)1000;
            g.scale      = grp["scale"]      | 1.0f;
            strlcpy(g.units,     grp["units"]      | "",      sizeof(g.units));
            g.mqttEnable = grp["mqttEnable"] | false;
            strlcpy(g.mqttTopic, grp["mqttTopic"]  | "",      sizeof(g.mqttTopic));

            uint8_t ri = 0;
            for (JsonVariant rv : grp["regs"].as<JsonArray>()) {
                if (ri >= MAX_REGS_PER_GROUP) break;
                const char* rn = rv.as<const char*>();
                strlcpy(g.regs[ri++], rn ? rn : "", REG_NAME_LEN);
            }

            for (JsonObject wg : grp["writes"].as<JsonArray>()) {
                if (g.writeCount >= MAX_WRITES_PER_GROUP) break;
                RemoteWriteGroupCfg_t& w = g.writes[g.writeCount];

                strlcpy(w.name,      wg["name"]       | "Write", sizeof(w.name));
                w.fc       = wg["fc"]       | (uint8_t)16;
                w.startReg = wg["startReg"] | (uint16_t)0;
                w.count    = wg["count"]    | (uint8_t)1;
                strlcpy(w.mqttTopic, wg["mqttTopic"]  | "",      sizeof(w.mqttTopic));

                uint8_t wi = 0;
                for (JsonVariant rv : wg["regs"].as<JsonArray>()) {
                    if (wi >= MAX_REGS_PER_GROUP) break;
                    const char* rn = rv.as<const char*>();
                    strlcpy(w.regs[wi++], rn ? rn : "", REG_NAME_LEN);
                }
                wi = 0;
                for (JsonVariant dv : wg["defaults"].as<JsonArray>()) {
                    if (wi >= MAX_REGS_PER_GROUP) break;
                    w.defaults[wi++] = (uint16_t)dv.as<int>();
                }
                g.writeCount++;
            }

            d.groupCount++;
        }

        _remoteCfg.deviceCount++;
    }

    _remoteCfg.loaded = true;
    Log(LOG, "FS: Remote.json loaded (%u device(s))\r\n", _remoteCfg.deviceCount);
}

void RemoteSaveConfig(const RemoteConfig_t* cfg) {
    DynamicJsonDocument doc(8192);
    doc["rev"] = "2.0";
    JsonArray devArr = doc.createNestedArray("devices");

    for (uint8_t i = 0; i < cfg->deviceCount; i++) {
        const RemoteDeviceCfg_t& d = cfg->devices[i];
        JsonObject dev = devArr.createNestedObject();
        dev["name"]    = d.name;
        dev["address"] = d.address;
        if (d.typeId)    dev["typeId"]         = d.typeId;
        if (d.swVersion) {
            dev["swVersion"]      = d.swVersion;
            dev["versionRegIdx"]  = d.versionRegIdx;
            dev["versionRegType"] = d.versionRegType;
        }

        JsonArray grpArr = dev.createNestedArray("groups");
        for (uint8_t g = 0; g < d.groupCount; g++) {
            const RemoteGroupCfg_t& grp = d.groups[g];
            JsonObject go = grpArr.createNestedObject();
            go["name"]       = grp.name;
            go["fc"]         = grp.fc;
            go["startReg"]   = grp.startReg;
            go["count"]      = grp.count;
            go["pollMs"]     = grp.pollMs;
            go["scale"]      = grp.scale;
            go["units"]      = grp.units;
            go["mqttEnable"] = grp.mqttEnable;
            go["mqttTopic"]  = grp.mqttTopic;
            JsonArray regs = go.createNestedArray("regs");
            for (uint8_t r = 0; r < grp.count && r < MAX_REGS_PER_GROUP; r++)
                regs.add(grp.regs[r]);

            if (grp.writeCount > 0) {
                JsonArray wArr = go.createNestedArray("writes");
                for (uint8_t w = 0; w < grp.writeCount; w++) {
                    const RemoteWriteGroupCfg_t& wg = grp.writes[w];
                    JsonObject wo = wArr.createNestedObject();
                    wo["name"]      = wg.name;
                    wo["fc"]        = wg.fc;
                    wo["startReg"]  = wg.startReg;
                    wo["count"]     = wg.count;
                    wo["mqttTopic"] = wg.mqttTopic;
                    JsonArray wregs = wo.createNestedArray("regs");
                    JsonArray wdefs = wo.createNestedArray("defaults");
                    for (uint8_t r = 0; r < wg.count && r < MAX_REGS_PER_GROUP; r++) {
                        wregs.add(wg.regs[r]);
                        wdefs.add(wg.defaults[r]);
                    }
                }
            }
        }
    }

    File f = LittleFS.open(Remotefilename, "w");
    if (!f) { Log(ERROR, "FS: cannot write Remote.json\r\n"); return; }
    if (serializeJson(doc, f) == 0)
        Log(ERROR, "FS: Remote.json write failed\r\n");
    else
        Log(LOG, "FS: Remote.json saved (%u device(s))\r\n", cfg->deviceCount);
    f.close();
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
