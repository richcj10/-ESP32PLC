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

    // Enable state persists in NVS (namespace "mods", key = module name).
    // Loaded in add(); a change via setEnabled() takes effect after restart.
    bool     isEnabled(uint8_t i) const { return (i < _count) && _enabled[i]; }
    bool     setEnabled(const char* name, bool en);

private:
    Module*  _mods[MODULE_MAX] = {};
    bool     _enabled[MODULE_MAX] = {};
    uint8_t  _count            = 0;
    uint32_t _lastMs[MODULE_MAX] = {};
};

extern ModuleManager modules;

// ── Self-registration ────────────────────────────────────────────────────────
// Put REGISTER_MODULE(instance) once in the module's .cpp. Which modules are
// built is chosen by `custom_modules` in platformio.ini (see scripts/modules.py).
// Runs during static init — add() only stores the pointer; NVS is read in begin().
struct ModuleRegistrar {
    explicit ModuleRegistrar(Module* m) { modules.add(m); }
};
#define REGISTER_MODULE(inst) static ModuleRegistrar _moduleRegistrar(&(inst))
