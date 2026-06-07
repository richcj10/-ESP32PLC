#include "TempHumid.h"
#include <Wire.h>
#include "Devices/Log.h"

TempHumidSensor climSensor;

// ── HDC2080 (0x40) ───────────────────────────────────────────────────────────
// Device ID registers 0xFE (low=0xD0) and 0xFF (high=0x07)

bool TempHumidSensor::_detectHDC2080() {
    Wire.beginTransmission(ADDR);
    Wire.write(0xFE);
    if (Wire.endTransmission() != 0) return false;
    Wire.requestFrom((uint8_t)ADDR, (uint8_t)2);
    if (Wire.available() < 2) return false;
    uint8_t id_lo = Wire.read();
    uint8_t id_hi = Wire.read();
    if (id_lo != 0xD0 || id_hi != 0x07) return false;
    Log(NOTIFY, "TempHumid: HDC2080 detected at 0x%02X\r\n", ADDR);
    _type = TH_HDC2080;
    return true;
}

bool TempHumidSensor::_readHDC2080() {
    // Trigger one-shot measurement
    Wire.beginTransmission(ADDR);
    Wire.write(0x0F);
    Wire.write(0x01);
    if (Wire.endTransmission() != 0) return false;
    delay(2);

    Wire.beginTransmission(ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;
    Wire.requestFrom((uint8_t)ADDR, (uint8_t)4);
    if (Wire.available() < 4) return false;

    uint16_t raw_t = (uint16_t)Wire.read() | ((uint16_t)Wire.read() << 8);
    uint16_t raw_h = (uint16_t)Wire.read() | ((uint16_t)Wire.read() << 8);
    float c = (raw_t / 65536.0f) * 165.0f - 40.0f;
    _tempF  = c * 1.8f + 32.0f;
    _humid  = (raw_h / 65536.0f) * 100.0f;
    return true;
}

// ── SI7021 (0x40) ────────────────────────────────────────────────────────────
// Chip ID command 0xFC 0xC9 → first byte is 0x15 for SI7021

bool TempHumidSensor::_detectSI7021() {
    Wire.beginTransmission(ADDR);
    Wire.write(0xFC);
    Wire.write(0xC9);
    if (Wire.endTransmission() != 0) return false;
    delay(5);
    Wire.requestFrom((uint8_t)ADDR, (uint8_t)6);
    if (Wire.available() < 1) return false;
    uint8_t id = Wire.read();
    while (Wire.available()) Wire.read();
    if (id != 0x15 && id != 0x14 && id != 0x0D) return false;
    Log(NOTIFY, "TempHumid: SI7021 detected (id=0x%02X) at 0x%02X\r\n", id, ADDR);
    _type = TH_SI7021;
    return true;
}

bool TempHumidSensor::_readSI7021() {
    // Humidity (hold master)
    Wire.beginTransmission(ADDR);
    Wire.write(0xE5);
    if (Wire.endTransmission() != 0) return false;
    delay(25);
    Wire.requestFrom((uint8_t)ADDR, (uint8_t)3);
    if (Wire.available() < 2) return false;
    uint16_t rh_raw = ((uint16_t)Wire.read() << 8) | (Wire.read() & 0xFC);
    if (Wire.available()) Wire.read();

    // Temperature (read from previous RH measurement, no new conversion)
    Wire.beginTransmission(ADDR);
    Wire.write(0xE0);
    if (Wire.endTransmission() != 0) return false;
    Wire.requestFrom((uint8_t)ADDR, (uint8_t)2);
    if (Wire.available() < 2) return false;
    uint16_t t_raw = ((uint16_t)Wire.read() << 8) | (Wire.read() & 0xFC);

    float c = ((175.72f * (float)t_raw) / 65536.0f) - 46.85f;
    _tempF  = c * 1.8f + 32.0f;
    _humid  = ((125.0f * (float)rh_raw) / 65536.0f) - 6.0f;
    if (_humid > 100.0f) _humid = 100.0f;
    if (_humid < 0.0f)   _humid = 0.0f;
    return true;
}

// ── Public API ───────────────────────────────────────────────────────────────

void TempHumidSensor::begin() {
    // Kick off first detection attempt; update() retries until MAX_FAILS
    _detectHDC2080() || _detectSI7021();
    if (_type != TH_NONE) _available = true;
}

void TempHumidSensor::update() {
    if (_failCount >= MAX_FAILS) return;

    if (_type == TH_NONE) {
        if (_detectHDC2080() || _detectSI7021()) {
            _available = true;
            _failCount = 0;
        } else if (++_failCount >= MAX_FAILS) {
            Log(ERROR, "TempHumid: no sensor found after %u attempts — stopping\r\n",
                (unsigned)MAX_FAILS);
        }
        return;
    }

    bool ok = (_type == TH_HDC2080) ? _readHDC2080() : _readSI7021();
    if (ok) {
        _failCount = 0;
        _available = true;
    } else if (++_failCount >= MAX_FAILS) {
        _available = false;
        Log(ERROR, "TempHumid: %u consecutive read failures — stopping\r\n",
            (unsigned)MAX_FAILS);
    }
}
