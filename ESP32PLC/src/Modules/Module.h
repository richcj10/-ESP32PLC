#pragma once
#include <ArduinoJson.h>

class AsyncWebServer;

class Module {
public:
    virtual ~Module() {}
    virtual const char* name() = 0;

    // ── Lifecycle ────────────────────────────────────────────────────────────
    virtual void begin()        {}
    virtual void startNetwork() {}
    virtual void update()       {}
    virtual uint32_t updateIntervalMs() { return 1000; }

    // ── MQTT ─────────────────────────────────────────────────────────────────
    virtual void mqttSubscribe()                                       {}
    virtual void mqttDiscovery()                                       {}
    virtual void mqttPublish()                                         {}
    virtual bool handleMQTT(const char* topic, const char* payload)    { (void)topic; (void)payload; return false; }

    // ── Live data (GET /api/modules/<name>/data) ─────────────────────────────
    virtual void getLiveData(JsonObject& out)                          { (void)out; }

    // ── Custom web routes — called once from ModuleManager::registerRoutes() ─
    virtual void registerRoutes(AsyncWebServer&)                       {}
};
