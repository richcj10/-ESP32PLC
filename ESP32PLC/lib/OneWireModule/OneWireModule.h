#pragma once
#include "Modules/Module.h"
#include <stdint.h>

class OneWire;

#define OW_MAX_SENSORS   8
#define OW_CONFIG_FILE   "/OneWire.json"
#define OW_DEFAULT_PIN   21           // used when /OneWire.json doesn't exist yet
#define OW_DEFAULT_TOPIC "onewire"    // -> ESPPLC/<host>/onewire/<sensor-id>
#define OW_TOPIC_LEN     32

// DS18B20 temperature sensors on a 1-Wire bus, reported to MQTT with Home
// Assistant discovery. Settings live in /OneWire.json on LittleFS:
//   {"enable":true,"pin":21,"mqttTopic":"onewire"}
// created with those defaults if missing, edited from the web I/O page.
//
// Reads are non-blocking: one "convert all" command, then each sensor's
// scratchpad is read on a later update() tick (one sensor per tick).
class OneWireModule : public Module {
public:
    const char* name() override { return "onewire"; }

    void begin()  override;
    void update() override;
    uint32_t updateIntervalMs() override { return 50; }

    void mqttDiscovery() override;
    void mqttPublish()   override;

    void getLiveData(JsonObject& out) override;
    void registerRoutes(AsyncWebServer& svr) override;

    static bool pinAllowed(int8_t pin);

private:
    struct Config {
        bool   enable = true;
        int8_t pin    = OW_DEFAULT_PIN;
        char   mqttTopic[OW_TOPIC_LEN + 1] = OW_DEFAULT_TOPIC;
    };

    struct Sensor {
        uint8_t  rom[8];
        char     id[17];        // ROM as hex, e.g. 28FF641E8316034D
        float    tempF;
        bool     valid;         // last read had a good CRC
        float    lastPubF;      // NAN = never published
        uint32_t lastPubMs;
        bool     discovered;    // HA discovery sent for this MQTT session
    };

    enum State : uint8_t { IDLE, CONVERTING, READING };

    Config   _cfg;
    OneWire* _bus      = nullptr;
    Sensor   _s[OW_MAX_SENSORS];
    uint8_t  _count    = 0;
    State    _state    = IDLE;
    uint32_t _stateMs  = 0;
    uint8_t  _readIdx  = 0;
    uint32_t _lastScanMs = 0;

    // Web requests are queued here and applied by update() on the main loop,
    // so the bus is never torn down while a read is in progress.
    Config        _pendingCfg;
    volatile bool _cfgPending    = false;
    volatile bool _pendingRescan = false;

    bool _loadConfig();                       // false = file missing/invalid (defaults kept)
    bool _saveConfig(const Config& c);
    void _apply();                            // (re)open or close the bus from _cfg
    void _closeBus();
    void _scan();
    bool _readSensor(Sensor& s);
    void _publishDiscovery(Sensor& s);
    void _stateTopic(const Sensor& s, char* out, size_t n) const;
    static void _cleanTopic(const char* in, char* out, size_t n);
};

extern OneWireModule owModule;
