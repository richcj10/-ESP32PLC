#include "ModuleManager.h"
#include <ArduinoJson.h>

ModuleManager modules;

void ModuleManager::add(Module* m) {
    if (_count < MODULE_MAX) {
        _mods[_count++] = m;
    }
}

void ModuleManager::begin() {
    for (uint8_t i = 0; i < _count; i++)
        _mods[i]->begin();
}

void ModuleManager::startNetwork() {
    for (uint8_t i = 0; i < _count; i++)
        _mods[i]->startNetwork();
}

void ModuleManager::update() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < _count; i++) {
        uint32_t interval = _mods[i]->updateIntervalMs();
        if (interval == 0 || (now - _lastMs[i]) >= interval) {
            _lastMs[i] = now;
            _mods[i]->update();
        }
    }
}

void ModuleManager::onMQTTConnect() {
    for (uint8_t i = 0; i < _count; i++) {
        _mods[i]->mqttSubscribe();
        _mods[i]->mqttDiscovery();
    }
}

bool ModuleManager::handleMQTT(const char* topic, const char* payload) {
    for (uint8_t i = 0; i < _count; i++) {
        if (_mods[i]->handleMQTT(topic, payload)) return true;
    }
    return false;
}

void ModuleManager::mqttPublish() {
    for (uint8_t i = 0; i < _count; i++)
        _mods[i]->mqttPublish();
}

void ModuleManager::registerRoutes(AsyncWebServer& svr) {
    for (uint8_t i = 0; i < _count; i++) {
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
