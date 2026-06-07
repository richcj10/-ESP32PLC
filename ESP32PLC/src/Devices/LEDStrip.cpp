#include "LEDStrip.h"
#include "Devices/Log.h"

LEDStrip ledStrip;

// ── DDP packet format ────────────────────────────────────────────────────────
// [0]   flags   (0x41 = version 1, push)
// [1]   sequence
// [2]   data type (0x02 = RGB)
// [3]   source/dest id
// [4-7] offset — byte offset into output buffer (big-endian uint32)
// [8-9] length  — data byte count (big-endian uint16)
// [10+] RGB data

void LEDStrip::processDDP(const uint8_t* buf, size_t len) {
    if (len < 10) return;
    uint32_t offset = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) |
                      ((uint32_t)buf[6] <<  8) |  (uint32_t)buf[7];
    uint16_t dlen   = ((uint16_t)buf[8] << 8) | buf[9];
    if (len < (size_t)(10 + dlen) || dlen == 0) return;

    const uint8_t* data     = buf + 10;
    uint16_t       startLed = (uint16_t)(offset / 3);

    for (uint16_t i = 0; i < dlen / 3; i++) {
        uint16_t idx = startLed + i;
        if (idx >= STRIP_NUM_LEDS) break;
        _leds[idx] = CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }
    _lastDDP = millis();
    FastLED.show();
}

// ── Public API ───────────────────────────────────────────────────────────────

void LEDStrip::begin() {
    FastLED.addLeds<STRIP_LED_TYPE, STRIP_PIN, STRIP_COLOR_ORDER>(_leds, STRIP_NUM_LEDS);
    FastLED.setBrightness(255);
    fill_solid(_leds, STRIP_NUM_LEDS, CRGB::Black);
    FastLED.show();

    if (_udp.listen(DDP_PORT)) {
        _udp.onPacket([this](AsyncUDPPacket pkt) {
            processDDP(pkt.data(), pkt.length());
        });
        Log(NOTIFY, "LED: DDP listener on port %u\r\n", DDP_PORT);
    } else {
        Log(ERROR, "LED: DDP listen failed\r\n");
    }
}

void LEDStrip::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
    _r = r; _g = g; _b = b; _bri = bri;
    _on    = true;
    _dirty = true;
}

void LEDStrip::setOff() {
    _on    = false;
    _dirty = true;
}

void LEDStrip::_applyMQTTColor() {
    if (!_on) {
        fill_solid(_leds, STRIP_NUM_LEDS, CRGB::Black);
    } else {
        CRGB c(_r, _g, _b);
        c.nscale8(_bri);
        fill_solid(_leds, STRIP_NUM_LEDS, c);
    }
    FastLED.show();
    _dirty = false;
}

void LEDStrip::update() {
    bool ddpActive = isDDPActive();

    if (ddpActive) {
        _ddpWasActive = true;
        return;  // DDP has control — don't touch the buffer
    }

    // DDP just went idle OR MQTT color changed — restore solid color
    if (_ddpWasActive || _dirty) {
        _ddpWasActive = false;
        _applyMQTTColor();
    }
}
