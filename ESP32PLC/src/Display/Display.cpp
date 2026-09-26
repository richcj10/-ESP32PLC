#include "Display.h"
#include <Arduino.h>
#include "UIPages.h"
#include "Define.h"
#include "HAL/Digital/Digital.h"
#include "Devices/JoyStick.h"
#include "Devices/Log.h"

// ── Hardware seam ─────────────────────────────────────────────────────────────
// Implemented in DisplayTFT.cpp or DisplayOLED.cpp — never both.
// Display.cpp calls these; nothing else should.
extern void _hw_init();
extern void _hw_clear();
extern void _hw_brightness(uint8_t b);      // OLED impl is a no-op
extern void _hw_boot_log(const char* line);
extern void _hw_ap_info(const char* ssid);

// ── State ─────────────────────────────────────────────────────────────────────
char DisplayMode  = 1;
char DisplaySleepEn = 1;

static bool _apMode = false;
static char _apSsid[40] = "";

unsigned long DisplayOnTime      = 0;
unsigned long LastDisplayUpdate  = 0;

// ── Setup ─────────────────────────────────────────────────────────────────────

void DisplaySetup() {
    _hw_init();
}

// ── Joystick-driven page carousel ────────────────────────────────────────────

static bool     _joySelPrev  = false;
static char     _joyPosPrev  = JOYSTICK_NONE;
static uint32_t _apSelHoldMs  = 0;
static uint32_t _selHoldMs    = 0;

void DisplayManager() {
    bool joySel  = GetJoyStickSelect();
    bool joyEdge = joySel && !_joySelPrev;
    _joySelPrev  = joySel;

    char joyPos      = GetJoyStickPos();
    bool joyRight    = (joyPos == JOYSTICK_RIGHT) && (_joyPosPrev != JOYSTICK_RIGHT);
    bool joyLeft     = (joyPos == JOYSTICK_LEFT)  && (_joyPosPrev != JOYSTICK_LEFT);
    _joyPosPrev      = joyPos;

    if (DisplayMode == 0) {
        if (joyEdge || joyRight || joyLeft) {
            Log(NOTIFY, "Display Wakeup");
            DisplayTimeoutReset();
            _hw_brightness(25);
            if (_apMode) { _hw_ap_info(_apSsid); } else { UIPageDraw(); }
            DisplayMode = 1;
        }
    } else {
        if (!_apMode) {
            bool lockNav = UIPageLockNav();
            if (joyRight && !lockNav) { DisplayTimeoutReset(); _selHoldMs = 0; UIPageNext(); }
            if (joyEdge)              { DisplayTimeoutReset(); _selHoldMs = 0; UIPageSelect(); }
            if (joyLeft  && !lockNav) { DisplayTimeoutReset(); _selHoldMs = 0; UIPagePrev(); }
            // Hold SELECT 3 s to execute a confirmed action (e.g. WiFi switch)
            if (joySel) {
                if (_selHoldMs == 0) _selHoldMs = millis();
                if (millis() - _selHoldMs >= 3000) {
                    _selHoldMs = 0;
                    UIPageExecuteHeld();
                }
            } else {
                _selHoldMs = 0;
            }
            if ((millis() - LastDisplayUpdate) > UIPageUpdateMs()) {
                LastDisplayUpdate = millis();
                UIPageDraw();
            }
        } else {
            if (joyEdge) DisplayTimeoutReset();
            // Hold SELECT 3 s to exit AP mode → STA
            if (joySel) {
                if (_apSelHoldMs == 0) _apSelHoldMs = millis();
                if (millis() - _apSelHoldMs >= 3000) UIApModeExitNow();
            } else {
                _apSelHoldMs = 0;
            }
        }
        DisplaySaver();
    }
}

// ── Screen saver ─────────────────────────────────────────────────────────────

void DisplaySaver() {
    if (DisplaySleepEn == 1) {
        unsigned long timeout = _apMode ? AP_SCREEN_TIMEOUT : SREENTIMEOUT;
        unsigned long elapsed = millis() - DisplayOnTime;
        if (elapsed > timeout) {
            Log(NOTIFY_FORCE, "DisplaySaver: SLEEP elapsed=%lu timeout=%lu\r\n", elapsed, timeout);
            DisplayMode = 0;
            _hw_clear();
            _hw_brightness(0);
        }
    }
}

// ── Thin dispatch wrappers ────────────────────────────────────────────────────

void DisplayClear()                { _hw_clear(); }
void DisplayLog(const char* t)     { _hw_boot_log(t); }
void DisplayAPInfo(const char* s)  { _hw_ap_info(s); }
void DisplayBrightnes(char b)      { _hw_brightness((uint8_t)b); }
void DisplayTimeoutReset()         { DisplayOnTime = millis(); }
void DispalySleepControl(char v)   { DisplaySleepEn = v; }
void DispalyConfigSet(char)        {}  // no-op: driver selected at compile time

void DisplaySetAPMode(bool ap, const char* ssid) {
    _apMode = ap;
    if (ap && ssid) strlcpy(_apSsid, ssid, sizeof(_apSsid));
}

void DisplayUploadStatus(const char* title, uint8_t pct, const char* msg) {
    DispalySleepControl(0);
    DisplayMode = 1;
    _hw_brightness(200);
    DisplayTimeoutReset();
    UIPageUpload(title, pct, msg);
}

void DisplayUploadDone(bool success, const char* msg) {
    UIPageUploadDone(success, msg);
    DisplayTimeoutReset();
    DispalySleepControl(1);
}
