#pragma once
#include <stdint.h>

// Unified temp/humidity HAL supporting HDC2080 and SI7021.
// Call begin() once after Wire is up, then update() on a slow timer.
// getTempF() / getHumidity() return -1 if the sensor is unavailable.
// After MAX_FAILS consecutive failures the sensor is permanently disabled.

enum TempHumidType : uint8_t { TH_NONE = 0, TH_HDC2080, TH_SI7021 };

class TempHumidSensor {
public:
    static constexpr uint8_t MAX_FAILS = 3;

    void begin();
    void update();

    float getTempF()          const { return _available ? _tempF : -1.0f; }
    float getHumidity()       const { return _available ? _humid : -1.0f; }
    bool  isAvailable()       const { return _available; }
    TempHumidType getType()   const { return _type; }

private:
    static constexpr uint8_t ADDR = 0x40;

    TempHumidType _type      = TH_NONE;
    bool          _available = false;
    uint8_t       _failCount = 0;
    float         _tempF     = -1.0f;
    float         _humid     = -1.0f;

    bool _detectHDC2080();
    bool _detectSI7021();
    bool _readHDC2080();
    bool _readSI7021();
};

extern TempHumidSensor climSensor;
