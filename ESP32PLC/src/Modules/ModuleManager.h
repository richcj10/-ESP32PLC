#pragma once
#include "Module.h"
#include <ESPAsyncWebServer.h>

#define MODULE_MAX 16

class ModuleManager {
public:
    void add(Module* m);

    void begin();
    void startNetwork();
    void update();

    void onMQTTConnect();
    bool handleMQTT(const char* topic, const char* payload);
    void mqttPublish();

    void registerRoutes(AsyncWebServer& server);

    uint8_t  count() const { return _count; }
    Module*  get(uint8_t i) const { return (i < _count) ? _mods[i] : nullptr; }

private:
    Module*  _mods[MODULE_MAX] = {};
    uint8_t  _count            = 0;
    uint32_t _lastMs[MODULE_MAX] = {};
};

extern ModuleManager modules;

// Call once before modules.begin() to register all concrete modules.
void RegisterModules();
