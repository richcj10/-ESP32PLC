#include "ModuleManager.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Devices/Log.h"

ModuleManager modules;

// Called from static initializers (REGISTER_MODULE) — `modules` has only
// constant member initializers, so it is ready before any registrar runs.
void ModuleManager::add(Module* m) {
    if (_count < MODULE_MAX) {
        _mods[_count++] = m;
    }
}

bool ModuleManager::setEnabled(const char* name, bool en) {
    for (uint8_t i = 0; i < _count; i++) {
        if (strcmp(_mods[i]->name(), name) != 0) continue;
        Preferences p;
        p.begin("mods", false);
        p.putBool(_mods[i]->name(), en);
        p.end();
        Log(NOTIFY, "Mod: %s %s (restart to apply)\r\n", name, en ? "enabled" : "disabled");
        return true;
    }
    return false;
}

void ModuleManager::begin() {
    // Enable state lives in NVS — not available during static init, so load here
    Preferences p;
    p.begin("mods", true);
    for (uint8_t i = 0; i < _count; i++)
        _enabled[i] = p.getBool(_mods[i]->name(), true);   // default: enabled
    p.end();

    for (uint8_t i = 0; i < _count; i++)
        if (_enabled[i]) _mods[i]->begin();
}

void ModuleManager::startNetwork() {
    for (uint8_t i = 0; i < _count; i++)
        if (_enabled[i]) _mods[i]->startNetwork();
}

void ModuleManager::update() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < _count; i++) {
        if (!_enabled[i]) continue;
        uint32_t interval = _mods[i]->updateIntervalMs();
        if (interval == 0 || (now - _lastMs[i]) >= interval) {
            _lastMs[i] = now;
            _mods[i]->update();
        }
    }
}

void ModuleManager::onMQTTConnect() {
    for (uint8_t i = 0; i < _count; i++) {
        if (!_enabled[i]) continue;
        _mods[i]->mqttSubscribe();
        _mods[i]->mqttDiscovery();
    }
}

bool ModuleManager::handleMQTT(const char* topic, const char* payload) {
    for (uint8_t i = 0; i < _count; i++) {
        if (_enabled[i] && _mods[i]->handleMQTT(topic, payload)) return true;
    }
    return false;
}

void ModuleManager::mqttPublish() {
    for (uint8_t i = 0; i < _count; i++)
        if (_enabled[i]) _mods[i]->mqttPublish();
}

void ModuleManager::registerRoutes(AsyncWebServer& svr) {
    /* GET /api/modules — compiled-in modules and their enable state
     * Response: [{"name":"ledstrip","enabled":true}, ...] */
    svr.on("/api/modules", HTTP_GET, [this](AsyncWebServerRequest* req) {
        AsyncResponseStream* resp = req->beginResponseStream("application/json");
        StaticJsonDocument<512> doc;
        JsonArray arr = doc.to<JsonArray>();
        for (uint8_t i = 0; i < _count; i++) {
            JsonObject o = arr.createNestedObject();
            o["name"]    = _mods[i]->name();
            o["enabled"] = _enabled[i];
        }
        serializeJson(doc, *resp);
        req->send(resp);
    });

    /* POST /api/modules/enable — Body: {"name":"fireworks","enabled":false}
     * Saved to NVS; takes effect after restart. */
    svr.on("/api/modules/enable", HTTP_POST,
        [](AsyncWebServerRequest*) {}, nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<96> doc;
            if (deserializeJson(doc, data, len) || !doc["name"].is<const char*>()) {
                req->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid JSON\"}");
                return;
            }
            if (!setEnabled(doc["name"], doc["enabled"] | true)) {
                req->send(404, "application/json", "{\"ok\":false,\"error\":\"unknown module\"}");
                return;
            }
            req->send(200, "application/json", "{\"ok\":true,\"restart\":true}");
        });

    // Disabled modules get no routes — e.g. /api/fireworks/fire does not exist.
    for (uint8_t i = 0; i < _count; i++) {
        if (!_enabled[i]) continue;
        Module* m = _mods[i];
        char path[48];
        snprintf(path, sizeof(path), "/api/modules/%s/data", m->name());

        svr.on(path, HTTP_GET, [m](AsyncWebServerRequest* req) {
            AsyncResponseStream* resp = req->beginResponseStream("application/json");
            StaticJsonDocument<256> doc;
            JsonObject obj = doc.to<JsonObject>();
            m->getLiveData(obj);
            serializeJson(doc, *resp);
            req->send(resp);
        });

        m->registerRoutes(svr);
    }
}
