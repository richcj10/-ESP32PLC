#pragma once
#include <FastLED.h>
#include <AsyncUDP.h>
#include <stdint.h>

#define STRIP_PIN      19
#define STRIP_NUM_LEDS 144   // change to match your strip length
#define STRIP_LED_TYPE WS2812B
#define STRIP_COLOR_ORDER GRB
#define DDP_PORT       4048
#define DDP_TIMEOUT_MS 5000  // revert to MQTT color after 5s of no DDP

class LEDStrip {
public:
    void begin();
    void update();              // call from main loop; applies MQTT color when DDP idle

    // Called by MQTT handler
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t bri = 255);
    void setOff();

    // State accessors for MQTT state publish
    bool    isOn()        const { return _on; }
    uint8_t r()           const { return _r; }
    uint8_t g()           const { return _g; }
    uint8_t b()           const { return _b; }
    uint8_t brightness()  const { return _bri; }
    bool    isDDPActive() const { return (millis() - _lastDDP) < DDP_TIMEOUT_MS; }

    // DDP packet entry point — called from AsyncUDP callback
    void processDDP(const uint8_t* buf, size_t len);

private:
    CRGB          _leds[STRIP_NUM_LEDS];
    uint8_t       _r = 255, _g = 255, _b = 255, _bri = 255;
    bool          _on          = false;
    bool          _dirty       = false;
    bool          _ddpWasActive = false;
    unsigned long _lastDDP     = 0;
    AsyncUDP      _udp;

    void _applyMQTTColor();
};

extern LEDStrip ledStrip;
