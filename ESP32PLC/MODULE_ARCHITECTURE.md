# Module Architecture

## Problem

Adding a new local hardware device (sensor, actuator, LED strip) currently requires touching
five files: `Functions.cpp`, `MQTT.cpp`, `Webportal.cpp`, `Sensors.cpp`, and `Main.html`.
This is error-prone and makes the codebase fragile for contributors unfamiliar with all the wiring.

## Goal

Adding a new device = **one new file**. No other files change.

---

## Module Base Class

```cpp
// src/Modules/Module.h

class Module {
public:
    virtual const char* name() = 0;  // unique slug, e.g. "ledstrip", "climate"

    // ── Lifecycle ────────────────────────────────────────────────────────────
    virtual void begin()        {}   // hardware init — before WiFi
    virtual void startNetwork() {}   // network init — after WiFi connects
    virtual void update()       {}   // called at updateIntervalMs() rate

    // ModuleManager calls update() no faster than this interval.
    // Default: 1000ms. Override to change rate.
    // Set to 0 = every loop (use when you need maximum rate or self-throttle inside update()).
    virtual uint32_t updateIntervalMs() { return 1000; }

    // ── MQTT ─────────────────────────────────────────────────────────────────
    virtual void mqttSubscribe()                                    {}
    // Called on every MQTT (re)connect — register command topics with the broker.
    // Example: mqttClient.subscribe("ESPPLC/hostname/ledstrip/set");

    virtual void mqttDiscovery()                                    {}
    // Called on every MQTT (re)connect — publish HA discovery config payloads.
    // Only needed if Home Assistant auto-discovery is used.

    virtual void mqttPublish()                                      {}
    // Called on the periodic publish tick — send current state to state topics.

    virtual bool handleMQTT(const char* topic, const char* payload) { return false; }
    // Called for every incoming message. Return true if consumed (stops propagation).

    // ── Web UI ───────────────────────────────────────────────────────────────
    // No hook needed. If /modules/<name>.html exists in LittleFS, the framework
    // serves it automatically at GET /api/modules/<name>/ui.
    // The file polls GET /api/modules/<name>/data for live values.
    // Upload or edit the file via the Files tab — no recompile required.

    // ── Live data ────────────────────────────────────────────────────────────
    virtual void getLiveData(JsonObject& out) {}
    // Writes the module's current values into the live data response.
    // This is what the Modules tab card uses to update its display.
    // Example:
    //   out["tempF"]    = 72.3;
    //   out["humidity"] = 45.1;
    // The web UI polls GET /api/modules/<name>/data and gets back {"tempF":72.3,...}
};
```

No module is required to implement every hook. Unused hooks cost nothing at runtime.

---

## Data Access API — Reading and Writing Remote Devices

Two levels of read API — pick the one that matches your use case:

```cpp
// src/Modules/PLCData.h

// ── Level 1: Raw — fastest, O(1) ─────────────────────────────────────────────
// addr     = Modbus slave address (e.g. 0x16)
// regIndex = zero-based index into that device's polled buffer
// Returns NAN if device not found or not yet polled.
float   plcReadRaw(uint8_t addr, uint8_t regIndex);

// ── Level 2: Address/index by name — resolve once at begin(), then use raw ───
// Translates a human name to the integer slave address or buffer index.
// Cache the returned value as uint8_t in a member variable.
// After a config change (e.g. reassigning a slave address), just reboot —
// begin() re-runs plcAddress/plcRegIndex and picks up the new value automatically.
// Returns 0xFF if not found (plcReadRaw treats an unknown address as NAN).
uint8_t plcAddress(const char* deviceName);              // "WeatherStation" → 0x16
uint8_t plcRegIndex(uint8_t addr, const char* regName);  // addr + "Temp"   → 2

// ── Convenience: By name every call — O(n) string search ─────────────────────
// Fine for one-off reads (begin(), getLiveData()). Not suitable for update() at 0ms.
float   plcRead(const char* deviceName, const char* regName);

// ── Writes ───────────────────────────────────────────────────────────────────
// Non-blocking — queued and executed on the next Modbus poll cycle.
void plcWrite(uint8_t addr, uint16_t reg, uint16_t value);
void plcWriteMulti(uint8_t addr, uint16_t startReg, const uint16_t* values, uint8_t count);
```

### Why integers rather than a buffer pointer

A cached `uint8_t` address is a value — it can never go stale.
A pointer into the live buffer would become invalid if the buffer is ever reallocated.
And when a device's slave address changes in Remote.json, `plcAddress()` picks up the
new value on the next boot with no module code changes.

### Usage pattern for a control module running at high rate

```cpp
class VentControlModule : public Module {
public:
    uint32_t updateIntervalMs() override { return 250; }

    void begin() override {
        _addr    = plcAddress("WeatherStation");    // string search once
        _tempReg = plcRegIndex(_addr, "Temp");      // string search once
    }

    void update() override {
        float t = plcReadRaw(_addr, _tempReg);      // O(1), no string search
        if (isnan(t)) return;
        bool open = (t > _setpoint);
        if (open != _relayOn) {
            plcWrite(_addr, 4, open ? 1 : 0);
            _relayOn = open;
        }
    }

private:
    uint8_t _addr    = 0xFF;
    uint8_t _tempReg = 0xFF;
    float   _setpoint = 85.0f;
    bool    _relayOn  = false;
};
```

No knowledge of polling loops, RemoteConfig, or MasterController needed.
`PLCData.cpp` is the only file that includes MasterController.h.

---

## Module UI — LittleFS HTML File

Each module can contribute a card to the **Modules tab** in the web portal by placing
an HTML file in LittleFS. No C++ changes needed to add or update the UI.

### How it works

1. Main.html's Modules tab calls `GET /api/modules` → list of registered module names
2. For each name, it fetches `GET /api/modules/<name>/ui`
3. ModuleManager checks if `/modules/<name>.html` exists in LittleFS and serves it
4. If no file exists, that module has no UI — silently skipped
5. The injected card polls `GET /api/modules/<name>/data` for live values

### What a module's HTML file looks like

```html
<!-- data/modules/climate.html -->
<div class="card">
  <div class="card-title">Climate Sensor</div>
  <div id="clim-tempF">--</div>
  <div id="clim-humid">--</div>
  <script>
    setInterval(function() {
      fetch('/api/modules/climate/data')
        .then(r => r.json())
        .then(d => {
          document.getElementById('clim-tempF').innerText = d.tempF.toFixed(1) + '°F';
          document.getElementById('clim-humid').innerText = d.humidity.toFixed(1) + '%';
        });
    }, 2000);
  </script>
</div>
```

- Edit the file and re-upload via the Files tab — no recompile
- Bundle it in `data/modules/` for the initial filesystem image
- The C++ module only implements `getLiveData()` — no HTML in code

### Endpoints added by ModuleManager automatically

| Endpoint                        | Returns                                              |
|---------------------------------|------------------------------------------------------|
| `GET /api/modules`              | `["ledstrip","climate"]`                             |
| `GET /api/modules/<name>/ui`    | Contents of `/modules/<name>.html` from LittleFS     |
| `GET /api/modules/<name>/data`  | JSON from `getLiveData()`                            |

ModuleManager registers these routes. No module touches Webportal.cpp.

---

## ModuleManager

```cpp
// src/Modules/ModuleManager.h

#define MAX_MODULES 16

class ModuleManager {
public:
    void add(Module* m);
    void registerRoutes();   // call from WebStart() — registers the /api/modules/* routes

    void begin();
    void startNetwork();
    void update();
    void onMQTTConnect();    // calls mqttSubscribe() then mqttDiscovery() on each module
    void mqttPublish();
    bool handleMQTT(const char* topic, const char* payload);
    // handleMQTT iterates modules until one returns true

private:
    Module*       _modules[MAX_MODULES]    = {};
    unsigned long _lastUpdate[MAX_MODULES] = {};
    uint8_t       _count = 0;
};

extern ModuleManager modules;
```

---

## Registration — Single File

```cpp
// src/Modules/Modules.cpp  <-- THE ONLY FILE YOU EDIT TO ADD/REMOVE A MODULE

#include "ModuleManager.h"
#include "Devices/LEDStripModule.h"
#include "Devices/TempHumidModule.h"

ModuleManager modules;

void RegisterModules() {
    modules.add(new LEDStripModule());
    modules.add(new TempHumidModule());
}
```

`RegisterModules()` is called once at the top of `SystemStart()`.

---

## Lifecycle — Where Hooks Are Called

| Hook              | Called from         | Timing                          |
|-------------------|---------------------|---------------------------------|
| `begin()`         | `SystemStart()`     | Boot, before WiFi               |
| `startNetwork()`  | `WiFiOK()`          | After WiFi connects             |
| `update()`        | `UIUpdateLoop()`    | Every main loop iteration       |
| `mqttSubscribe()` | `MQTTreconnect()`   | On each MQTT connect — subscribe to command topics |
| `mqttDiscovery()` | `MQTTreconnect()`   | On each MQTT connect — publish HA discovery config |
| `mqttPublish()`   | MQTT publish tick   | Periodic (existing timer)                          |
| `handleMQTT()`    | MQTT callback       | On every incoming MQTT message                     |
| `registerRoutes()`| `WebStart()`        | Once, before server.begin()     |

---

## What Changes in Existing Framework Files

| File              | Change                                                               |
|-------------------|----------------------------------------------------------------------|
| `Functions.cpp`   | `modules.begin()`, `modules.startNetwork()`, `modules.update()`                |
| `MQTT.cpp`        | `modules.onMQTTConnect()`, `modules.mqttPublish()`, `modules.handleMQTT()`     |
| `Webportal.cpp`   | `modules.registerRoutes()` — all /api/modules/* handled internally  |
| `Sensors.cpp`     | Gutted — logic moves into `TempHumidModule`                         |

These files never change again when a new module is added.

---

## Example: LEDStripModule

```cpp
// src/Devices/LEDStripModule.h

class LEDStripModule : public Module {
public:
    const char* name() override { return "ledstrip"; }

    void begin()        override { ledStrip.begin(); }
    void startNetwork() override { ledStrip.startUDP(); }
    void update()       override { ledStrip.update(); }

    void mqttSubscribe() override {
        mqttClient.subscribe("ESPPLC/hostname/ledstrip/set");
    }
    void mqttDiscovery() override { /* publish HA light entity config */ }
    void mqttPublish()   override { /* publish state topic */ }

    bool handleMQTT(const char* topic, const char* payload) override {
        // return true if this was our set topic
        return _handleStripWrite(topic, payload);
    }

    void getLiveData(JsonObject& out) override {
        out["on"]  = ledStrip.isOn();
        out["r"]   = ledStrip.r();
        out["g"]   = ledStrip.g();
        out["b"]   = ledStrip.b();
        out["bri"] = ledStrip.brightness();
        out["ddp"] = ledStrip.isDDPActive();
    }
    // UI: data/modules/ledstrip.html in LittleFS
};
```

---

## Example: TempHumidModule

```cpp
// src/Devices/TempHumidModule.h

class TempHumidModule : public Module {
public:
    const char* name() override { return "climate"; }

    void begin()  override { climSensor.begin(); }
    void update() override { climSensor.update(); }

    void mqttDiscovery() override { /* HA sensor entities for tempF + humidity */ }
    void mqttPublish()   override {
        if (!climSensor.isAvailable()) return;
        // publish tempF, humidity
    }

    void getLiveData(JsonObject& out) override {
        out["tempF"]    = climSensor.getTempF();
        out["humidity"] = climSensor.getHumidity();
        out["ok"]       = climSensor.isAvailable();
    }
    // UI: data/modules/climate.html in LittleFS
};
```

---

## Example: A Control Module (using plcRead / plcWrite)

```cpp
// src/Devices/VentControlModule.h
// Reads WeatherStation temp, opens vent relay when hot.

class VentControlModule : public Module {
public:
    const char* name() override { return "ventctrl"; }
    uint32_t updateIntervalMs() override { return 250; }

    void begin() override {
        _addr    = plcAddress("WeatherStation");    // string search once
        _tempReg = plcRegIndex(_addr, "Temp");      // string search once
    }

    void update() override {
        float t = plcReadRaw(_addr, _tempReg);      // O(1), no string search
        if (isnan(t)) return;
        bool shouldOpen = (t > _setpoint);
        if (shouldOpen != _relayOn) {
            plcWrite(_addr, 4, shouldOpen ? 1 : 0);
            _relayOn = shouldOpen;
        }
    }

    void getLiveData(JsonObject& out) override {
        out["setpoint"] = _setpoint;
        out["relay"]    = _relayOn;
        out["input"]    = plcReadRaw(_addr, _tempReg);
    }

private:
    uint8_t _addr     = 0xFF;
    uint8_t _tempReg  = 0xFF;
    float   _setpoint = 85.0f;
    bool    _relayOn  = false;
};
```

No knowledge of polling loops, RemoteConfig, or MasterController needed.

---

## Config Persistence

**LittleFS only.** Each module that needs to save settings reads/writes `/modules/<name>.json`.

```cpp
void VentControlModule::loadConfig() {
    File f = LittleFS.open("/modules/ventctrl.json", "r");
    if (!f) return;
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok)
        _setpoint = doc["setpoint"] | 85.0f;
    f.close();
}

void VentControlModule::saveConfig() {
    File f = LittleFS.open("/modules/ventctrl.json", "w");
    StaticJsonDocument<256> doc;
    doc["setpoint"] = _setpoint;
    serializeJson(doc, f);
    f.close();
}
```

- No size limit (unlike NVS ~4 KB namespace cap)
- Human readable — downloadable and editable from the existing Files tab
- Factory reset wipes `/modules/` along with everything else — consistent behaviour
- Same infrastructure already used by Remote.json — no new patterns to learn

---

## MQTT Topic Convention

```
ESPPLC/<hostname>/<module_name>/...
```

Examples:
```
ESPPLC/ESP32PLC-1234/ledstrip/state
ESPPLC/ESP32PLC-1234/ledstrip/set
ESPPLC/ESP32PLC-1234/climate/tempF
ESPPLC/ESP32PLC-1234/ventctrl/relay
```

---

## What a New Module Looks Like

1. Create `src/Devices/MyThingModule.h`
2. Implement the hooks you need
3. Add one line to `src/Modules/Modules.cpp`

That's it.

---

## Open Questions — Resolved

| # | Question | Decision |
|---|----------|----------|
| 1 | MAX_MODULES 16 | 16 is fine — 16 local hardware modules is plenty. Static array, no heap fragmentation. |
| 2 | MQTT subscription hook | Yes — `bool handleMQTT(topic, payload)` returns true if consumed. Added to base class. |
| 3 | Module ordering | Registration order in Modules.cpp is init order. No dependency declarations — keep it simple. |
| 4 | getLiveData scope | Separate endpoint per module: `GET /api/modules/<name>/data`. Not merged with Modbus device data. |
| 5 | Config persistence | NVS namespace per module (Option A). LittleFS fallback for large config. |
