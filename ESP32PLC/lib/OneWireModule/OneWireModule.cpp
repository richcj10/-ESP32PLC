#include "OneWireModule.h"
#include <Arduino.h>
#include <OneWire.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <math.h>
#include <new>
#include "Modules/ModuleManager.h"
#include "MQTT.h"
#include "FileSystem/FSInterface.h"
#include "Devices/Log.h"

OneWireModule owModule;
REGISTER_MODULE(owModule);

#define OW_READ_PERIOD_MS   5000UL            // start a conversion every 5 s
#define OW_CONVERT_MS        800UL            // DS18B20 12-bit conversion ≤ 750 ms
#define OW_RESCAN_MS      300000UL            // look for added/removed sensors every 5 min
#define OW_PUB_DELTA_F         0.2f           // publish when the value moves this much…
#define OW_PUB_HEARTBEAT_MS 60000UL           // …or at least once a minute

// GPIOs already used on this board (TFT, RS485, I2C, LEDs, shield I/O, joystick),
// plus flash/PSRAM (26-32), UART0 console (43/44) and boot-strap pins.
static const int8_t kUsedPins[] = {
    0, 1, 2, 3, 4, 5, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
    26, 27, 28, 29, 30, 31, 32, 38, 39, 40, 41, 42, 43, 44, 45, 46, 48
};

bool OneWireModule::pinAllowed(int8_t pin) {
    if (pin < 0 || pin > 48) return false;
    for (int8_t p : kUsedPins) if (p == pin) return false;
    return true;
}

// ── Config file ────────────────────────────────────────────────────────────

// Keep [A-Za-z0-9_-/], drop MQTT wildcards and spaces, trim leading/trailing '/'.
void OneWireModule::_cleanTopic(const char* in, char* out, size_t n) {
    size_t j = 0;
    for (size_t i = 0; in && in[i] && j + 1 < n; i++) {
        char c = in[i];
        if (isalnum((unsigned char)c) || c == '_' || c == '-' || c == '/') {
            if (c == '/' && (j == 0 || out[j - 1] == '/')) continue;   // no leading or double '/'
            out[j++] = c;
        }
    }
    while (j > 0 && out[j - 1] == '/') j--;
    out[j] = '\0';
    if (j == 0) strlcpy(out, OW_DEFAULT_TOPIC, n);
}

bool OneWireModule::_loadConfig() {
    File f = LittleFS.open(OW_CONFIG_FILE, "r");
    if (!f) return false;
    StaticJsonDocument<192> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) { Log(ERROR, "OneWire: %s invalid (%s) — using defaults", OW_CONFIG_FILE, err.c_str()); return false; }
    _cfg.enable = doc["enable"] | true;
    _cfg.pin    = doc["pin"]    | (int8_t)OW_DEFAULT_PIN;
    _cleanTopic(doc["mqttTopic"] | OW_DEFAULT_TOPIC, _cfg.mqttTopic, sizeof(_cfg.mqttTopic));
    return true;
}

bool OneWireModule::_saveConfig(const Config& c) {
    StaticJsonDocument<192> doc;
    doc["enable"]    = c.enable;
    doc["pin"]       = c.pin;
    doc["mqttTopic"] = c.mqttTopic;
    File f = LittleFS.open(OW_CONFIG_FILE, "w");
    if (!f) { Log(ERROR, "OneWire: cannot write %s", OW_CONFIG_FILE); return false; }
    serializeJsonPretty(doc, f);
    f.close();
    return true;
}

// ── Bus ────────────────────────────────────────────────────────────────────

void OneWireModule::_closeBus() {
    if (_bus) { _bus->~OneWire(); free(_bus); _bus = nullptr; }
    _count = 0;
    _state = IDLE;
}

void OneWireModule::_apply() {
    _closeBus();
    if (!_cfg.enable) { Log(LOG, "OneWire: disabled in %s", OW_CONFIG_FILE); return; }
    if (!pinAllowed(_cfg.pin)) { Log(ERROR, "OneWire: pin %d is in use — bus not started", (int)_cfg.pin); return; }
    void* mem = malloc(sizeof(OneWire));
    if (!mem) return;
    _bus = new (mem) OneWire((uint8_t)_cfg.pin);
    _scan();
}

void OneWireModule::begin() {
    if (!_loadConfig()) {
        _saveConfig(_cfg);                   // first boot: write the defaults so the file exists
        Log(LOG, "OneWire: created %s with defaults", OW_CONFIG_FILE);
    }
    _apply();
}

void OneWireModule::_scan() {
    if (!_bus) return;
    _count = 0;
    uint8_t rom[8];
    _bus->reset_search();
    while (_count < OW_MAX_SENSORS && _bus->search(rom)) {
        if (OneWire::crc8(rom, 7) != rom[7]) continue;
        if (rom[0] != 0x28 && rom[0] != 0x22 && rom[0] != 0x10) continue;   // DS18B20 / DS1822 / DS18S20
        Sensor& s = _s[_count++];
        memcpy(s.rom, rom, 8);
        for (uint8_t i = 0; i < 8; i++) snprintf(s.id + i * 2, 3, "%02X", rom[i]);
        s.tempF = NAN; s.valid = false; s.lastPubF = NAN; s.lastPubMs = 0; s.discovered = false;
    }
    _bus->reset_search();
    _lastScanMs = millis();
    Log(LOG, "OneWire: pin %d — %u sensor(s) found", (int)_cfg.pin, (unsigned)_count);
}

// ── Reading (non-blocking state machine) ──────────────────────────────────

bool OneWireModule::_readSensor(Sensor& s) {
    uint8_t d[9];
    if (!_bus->reset()) return false;
    _bus->select(s.rom);
    _bus->write(0xBE);                       // read scratchpad
    for (uint8_t i = 0; i < 9; i++) d[i] = _bus->read();
    if (OneWire::crc8(d, 8) != d[8]) return false;

    int16_t raw = (int16_t)((d[1] << 8) | d[0]);
    if (s.rom[0] == 0x10) {                  // DS18S20: 9-bit + count-remain
        raw <<= 3;
        if (d[7] == 0x10) raw = (raw & 0xFFF0) + 12 - d[6];
    }
    float c = raw / 16.0f;
    if (c < -55.0f || c > 125.0f || raw == 0x0550) return false;   // out of range / power-on 85 °C
    s.tempF = c * 1.8f + 32.0f;
    return true;
}

void OneWireModule::update() {
    // Apply web requests here, on the main loop
    if (_cfgPending) {
        _cfgPending = false;
        bool topicChanged = strcmp(_cfg.mqttTopic, _pendingCfg.mqttTopic) != 0;
        bool busChanged   = _cfg.enable != _pendingCfg.enable || _cfg.pin != _pendingCfg.pin;
        _cfg = _pendingCfg;
        if (busChanged) _apply();
        if (busChanged || topicChanged) mqttDiscovery();   // re-announce with the new state topic
    } else if (_pendingRescan) {
        _pendingRescan = false;
        _scan();
        mqttDiscovery();
    }

    if (!_bus) return;
    uint32_t now = millis();

    switch (_state) {
    case IDLE:
        if (now - _lastScanMs >= OW_RESCAN_MS) _scan();
        if (_count == 0 || now - _stateMs < OW_READ_PERIOD_MS) return;
        if (!_bus->reset()) { for (uint8_t i = 0; i < _count; i++) _s[i].valid = false; _stateMs = now; return; }
        _bus->skip();
        _bus->write(0x44, 1);                // convert T on every sensor at once
        _state = CONVERTING; _stateMs = now;
        break;

    case CONVERTING:
        if (now - _stateMs < OW_CONVERT_MS) return;
        _readIdx = 0; _state = READING;
        break;

    case READING:                            // one sensor per tick keeps each tick short
        if (_readIdx < _count) {
            Sensor& s = _s[_readIdx++];
            s.valid = _readSensor(s);
            return;
        }
        _state = IDLE; _stateMs = now;
        break;
    }
}

// ── MQTT ───────────────────────────────────────────────────────────────────

static void _safeId(const char* in, char* out, size_t n) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < n; i++) {
        char c = in[i];
        out[j++] = isalnum((unsigned char)c) ? (char)tolower(c) : '_';
    }
    out[j] = '\0';
}

void OneWireModule::_stateTopic(const Sensor& s, char* out, size_t n) const {
    snprintf(out, n, "%s/%s/%s", GetMQTTBaseTopic(), _cfg.mqttTopic, s.id);
}

void OneWireModule::_publishDiscovery(Sensor& s) {
    char host[32]; _safeId(GetHostName().c_str(), host, sizeof(host));
    char uid[64];  snprintf(uid, sizeof(uid), "espplc_%s_ow_%s", host, s.id);
    char topic[112]; snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/config", uid);
    char stat[128];  _stateTopic(s, stat, sizeof(stat));
    char payload[480];
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Temp %s\",\"stat_t\":\"%s\",\"unit_of_meas\":\"\xC2\xB0""F\","
        "\"dev_cla\":\"temperature\",\"stat_cla\":\"measurement\",\"uniq_id\":\"%s\","
        "\"val_tpl\":\"{{value|float}}\",\"exp_aft\":300,"
        "\"dev\":{\"ids\":[\"espplc_%s\"],\"name\":\"%s\",\"mf\":\"ESP32PLC\"}}",
        s.id + 12, stat, uid, host, GetHostName().c_str());
    s.discovered = GetMQTTClient().publish(topic, payload, true);
}

void OneWireModule::mqttDiscovery() {
    for (uint8_t i = 0; i < _count; i++) { _s[i].discovered = false; _s[i].lastPubF = NAN; }
}

void OneWireModule::mqttPublish() {
    if (!GetMQTTStatus()) return;
    uint32_t now = millis();
    for (uint8_t i = 0; i < _count; i++) {
        Sensor& s = _s[i];
        if (!s.discovered) _publishDiscovery(s);
        if (!s.valid) continue;
        bool due = isnan(s.lastPubF) || fabsf(s.tempF - s.lastPubF) >= OW_PUB_DELTA_F ||
                   now - s.lastPubMs >= OW_PUB_HEARTBEAT_MS;
        if (!due) continue;
        char topic[128]; _stateTopic(s, topic, sizeof(topic));
        char val[12];    snprintf(val, sizeof(val), "%.2f", s.tempF);
        if (GetMQTTClient().publish(topic, val)) { s.lastPubF = s.tempF; s.lastPubMs = now; }
    }
}

// ── Web ────────────────────────────────────────────────────────────────────

void OneWireModule::getLiveData(JsonObject& out) {
    out["enable"] = _cfg.enable;
    out["pin"]    = _cfg.pin;
    out["count"]  = _count;
}

void OneWireModule::registerRoutes(AsyncWebServer& svr) {
    /* GET /api/onewire — settings, usable pins, and sensors {id, tempF, ok} */
    svr.on("/api/onewire", HTTP_GET, [this](AsyncWebServerRequest* req) {
        AsyncResponseStream* resp = req->beginResponseStream("application/json");
        StaticJsonDocument<1280> doc;
        doc["enable"]    = _cfg.enable;
        doc["pin"]       = _cfg.pin;
        doc["mqttTopic"] = _cfg.mqttTopic;
        doc["topicBase"] = GetMQTTBaseTopic();
        doc["running"]   = _bus != nullptr;
        JsonArray free = doc.createNestedArray("free_pins");
        for (int8_t p = 0; p <= 48; p++) if (pinAllowed(p)) free.add(p);
        JsonArray arr = doc.createNestedArray("sensors");
        for (uint8_t i = 0; i < _count; i++) {
            JsonObject o = arr.createNestedObject();
            o["id"] = _s[i].id;
            o["ok"] = _s[i].valid;
            if (_s[i].valid) o["tempF"] = serialized(String(_s[i].tempF, 2));
        }
        serializeJson(doc, *resp);
        req->send(resp);
    });

    /* POST /api/onewire
     *   {"enable":true,"pin":21,"mqttTopic":"onewire"}  any subset — saved to /OneWire.json, applied live
     *   {"rescan":true}                                   look for sensors again */
    svr.on("/api/onewire", HTTP_POST,
        [](AsyncWebServerRequest*) {}, nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<192> doc;
            if (deserializeJson(doc, data, len)) {
                req->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid JSON\"}");
                return;
            }
            if (doc["rescan"] | false) {
                _pendingRescan = true;
                req->send(200, "application/json", "{\"ok\":true}");
                return;
            }
            Config c = _cfg;
            if (doc.containsKey("enable"))    c.enable = doc["enable"].as<bool>();
            if (doc.containsKey("pin"))       c.pin    = doc["pin"].as<int8_t>();
            if (doc.containsKey("mqttTopic")) _cleanTopic(doc["mqttTopic"] | "", c.mqttTopic, sizeof(c.mqttTopic));
            if (c.enable && !pinAllowed(c.pin)) {
                req->send(400, "application/json", "{\"ok\":false,\"error\":\"pin is in use\"}");
                return;
            }
            if (!_saveConfig(c)) {
                req->send(500, "application/json", "{\"ok\":false,\"error\":\"could not save config\"}");
                return;
            }
            _pendingCfg = c;
            _cfgPending = true;                // applied by update() on the main loop
            char buf[80];
            snprintf(buf, sizeof(buf), "{\"ok\":true,\"mqttTopic\":\"%s\"}", c.mqttTopic);
            req->send(200, "application/json", buf);
        });
}
