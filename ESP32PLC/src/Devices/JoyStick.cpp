#include "JoyStick.h"
#include <Arduino.h>
#include <Preferences.h>
#include "Devices/Log.h"

// ── Default map — hardware observed: UP~0, RIGHT~1960, DOWN~2640, LEFT~2990, NONE~3220 ──
static JoyMap _map = {
    { 3106, 4095 },  // none
    {    0,  980 },  // up
    { 2301, 2815 },  // down
    { 2816, 3105 },  // left
    {  981, 2300 },  // right
};

static int     _average = 3220;
static int     _window[3] = { 3220, 3220, 3220 };
static uint8_t _winIdx  = 0;

static char          _btnRaw     = 0;
static char          _btnStatus  = 0;
static unsigned long _lastBtnTime = 0;
static long          _lastUpdateMs = 0;

// ── Public API ────────────────────────────────────────────────────────────────

void JoyStickStart() {
    pinMode(1, INPUT);
    pinMode(0, INPUT);
    _btnRaw = _btnStatus = (char)digitalRead(0);
    _lastBtnTime = millis();
    int v = analogRead(1);
    _window[0] = _window[1] = _window[2] = v;
    _average = v;
    // Saved map intentionally not loaded — always boot with defaults.
}

void JoyStickUpdate() {
    unsigned long now = millis();

    char rawBtn = digitalRead(0);
    if (rawBtn != _btnRaw) _lastBtnTime = now;
    _btnRaw = rawBtn;
    if ((now - _lastBtnTime) > 50 && rawBtn != _btnStatus)
        _btnStatus = rawBtn;

    if (now - _lastUpdateMs >= 80) {
        _lastUpdateMs = now;
        _window[_winIdx] = analogRead(1);
        _winIdx = (_winIdx + 1) % 3;
        _average = (_window[0] + _window[1] + _window[2]) / 3;
    }
}

char GetJoyStickPos() {
    int v = _average;
    if (v >= _map.none.lo  && v <= _map.none.hi)  return JOYSTICK_NONE;
    if (v >= _map.up.lo    && v <= _map.up.hi)    return JOYSTICK_UP;
    if (v >= _map.down.lo  && v <= _map.down.hi)  return JOYSTICK_DOWN;
    if (v >= _map.left.lo  && v <= _map.left.hi)  return JOYSTICK_LEFT;
    if (v >= _map.right.lo && v <= _map.right.hi) return JOYSTICK_RIGHT;
    return JOYSTICK_ERROR;
}

char GetJoyStickSelect() { return !_btnStatus; }

int  JoyStickRawAvg()             { return _average; }
JoyMap JoyStickGetMap()           { return _map; }
void   JoyStickSetMap(const JoyMap& m) { _map = m; }

void JoyStickSaveMap() {
    Preferences prefs;
    prefs.begin("joystick", false);
    prefs.putBytes("map", &_map, sizeof(_map));
    prefs.end();
    Serial.printf("\r\n=== JoyStick calibration saved ===\r\n");
    Serial.printf("  NONE : %4d .. %4d\r\n", _map.none.lo,  _map.none.hi);
    Serial.printf("  UP   : %4d .. %4d\r\n", _map.up.lo,    _map.up.hi);
    Serial.printf("  DOWN : %4d .. %4d\r\n", _map.down.lo,  _map.down.hi);
    Serial.printf("  LEFT : %4d .. %4d\r\n", _map.left.lo,  _map.left.hi);
    Serial.printf("  RIGHT: %4d .. %4d\r\n", _map.right.lo, _map.right.hi);
    Serial.printf("==================================\r\n\r\n");
}

void JoyStickLoadMap() {
    Preferences prefs;
    prefs.begin("joystick", true);
    if (prefs.getBytesLength("map") == sizeof(_map)) {
        prefs.getBytes("map", &_map, sizeof(_map));
        Log(NOTIFY, "JoyStick: map loaded from NVS\r\n");
    } else {
        Log(NOTIFY, "JoyStick: no saved map — using defaults\r\n");
    }
    prefs.end();
}

void GetJoystickPrint(char x) {
    switch (x) {
        case JOYSTICK_NONE:  break;
        case JOYSTICK_UP:    Serial.println("Up");    break;
        case JOYSTICK_DOWN:  Serial.println("Down");  break;
        case JOYSTICK_LEFT:  Serial.println("Left");  break;
        case JOYSTICK_RIGHT: Serial.println("Right"); break;
        default:                                      break;
    }
}
