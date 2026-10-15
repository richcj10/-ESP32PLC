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

    // true: update() runs in the high-priority control task (with Modbus and the
    // inputs), so WiFi/MQTT/display stalls in loop() can't delay it — for
    // time-critical work like firing sequences. It must not use MQTT, FastLED or
    // the display from update() (those belong to loop()).
    virtual bool realtime() { return false; }

    // true while the module is doing something a reboot must not interrupt
    // (e.g. a firing sequence). Automatic reboots wait until no module is busy.
    virtual bool busy() { return false; }

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
