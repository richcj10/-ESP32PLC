#include "Digital.h"
#include <Arduino.h>
#include "Define.h"
#include "../DeviceConfig.h"
#include "Devices/Log.h"

/* ── Private state ───────────────────────────────────────────────────────── */

static uint8_t _inPins[8]   = {};
static uint8_t _inCount     = 0;
static bool    _inState[8]  = {};

static uint8_t _outPins[8]  = {};
static uint8_t _outCount    = 0;

static bool         _userLED    = false;
static bool         _swLast     = false;
static bool         _swVal      = false;
static unsigned long _swDebounce = 0;

/* ── MCU fixed pins ──────────────────────────────────────────────────────── */

void DigitalStart(void) {
    pinMode(LED,     OUTPUT);
    pinMode(USER_SW, INPUT);
}

/* ── Shield I/O init ─────────────────────────────────────────────────────── */

static void _addInput(uint8_t pin) {
    if (_inCount < 8) { _inPins[_inCount++] = pin; }
}

static void _addOutput(uint8_t pin) {
    if (_outCount < 8) { _outPins[_outCount++] = pin; }
}

void IOStart(void) {
    _inCount  = 0;
    _outCount = 0;

    switch (GetDeviceType()) {

    case 1:  /* ESP32-S3 standard shield */
        _addInput(MP1INPUT);   /* 45 */
        _addInput(MP2INPUT);   /* 48 */
        _addInput(MP3INPUT);   /* 13 */
        _addInput(MP4INPUT);   /* 14 */
        _addOutput(MP1OUT);    /* 39 */
        _addOutput(MP2OUT);    /* 41 */
        _addOutput(MP3OUT);    /* 42 */
        _addOutput(MP4OUT);    /* 40 */
        _addOutput(MP5OUT);    /* 38 */
        break;

    default:
        break;
    }

    for (uint8_t i = 0; i < _inCount;  i++) pinMode(_inPins[i],  INPUT);
    for (uint8_t i = 0; i < _outCount; i++) pinMode(_outPins[i], OUTPUT);
}

/* ── Main-loop scan ──────────────────────────────────────────────────────── */

void ScanIO(void) {
    for (uint8_t i = 0; i < _inCount; i++)
        _inState[i] = digitalRead(_inPins[i]);
}

/* ── Accessors ───────────────────────────────────────────────────────────── */

uint8_t GetInputCount(void)  { return _inCount; }
uint8_t GetOutputCount(void) { return _outCount; }

bool GetInput(uint8_t n) {
    return (n < _inCount) && _inState[n];
}

bool GetOutput(uint8_t n) {
    return (n < _outCount) && (bool)digitalRead(_outPins[n]);
}

void SetOutput(uint8_t n, bool val) {
    if (n < _outCount) digitalWrite(_outPins[n], val ? HIGH : LOW);
}

/* ── MCU user controls ───────────────────────────────────────────────────── */

void SetUserLED(bool val) {
    _userLED = val;
    digitalWrite(LED, _userLED);
}

void ToggletUserLED(void) {
    _userLED = !_userLED;
    digitalWrite(LED, _userLED);
}

void ScanUserInput(void) {
    bool val = (bool)digitalRead(USER_SW);
    if (_swLast != val) _swDebounce = millis();
    if ((millis() - _swDebounce) > 200) _swVal = val;
    _swLast = val;
}

bool GetUserSWValue(void) { return !_swVal; }

/* ── Legacy shim ─────────────────────────────────────────────────────────── */

void ScanArrayAdd(char /*pin*/) {}  /* no-op; GPIOStart() path is superseded by IOStart() */
