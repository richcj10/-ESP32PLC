#include "Display.h"
#include <WiFi.h>
#include "UIPages.h"
#include "Define.h"
#include "HAL/Digital/Digital.h"
#include "MQTT.h"
#include "Devices/JoyStick.h"
#include "Devices/Log.h"

// ── Hardware seam ─────────────────────────────────────────────────────────────
// Implemented in DisplayTFT.cpp or DisplayOLED.cpp — never both.
// Display.cpp calls these; nothing else should.
extern void _hw_init();
extern void _hw_clear();
extern void _hw_brightness(uint8_t b);      // OLED impl is a no-op
extern void _hw_wifi_signal(uint8_t lvl);   // 0=animation frame, 1-3=strength
extern void _hw_mqtt_icon(uint8_t mode);
extern void _hw_th_bar();
extern void _hw_id_label();
extern void _hw_logo();
extern void _hw_boot_log(const char* line);
extern void _hw_ap_info(const char* ssid);
extern void _hw_center_input();
extern void _hw_center_output();
extern void _hw_center_ip();
extern void _hw_center_remote();

// ── State ─────────────────────────────────────────────────────────────────────
char DisplayMode  = 1;
char ModeActive   = 1;
char ScreenShow   = 0;
char DisplaySleepEn = 1;

static bool _apMode = false;
static char _apSsid[40] = "";

unsigned long TimeReading        = 0;
unsigned long DisplayOnTime      = 0;
unsigned long LastDisplayUpdate  = 0;

long          DisplayRefreshRate    = 0;
long          DisplayUpdateInterval = 1000;
unsigned long DisplaycurrentMillis  = 0;

// ── Setup ─────────────────────────────────────────────────────────────────────

void DisplaySetup() {
    _hw_init();
}

// ── Joystick-driven page carousel ────────────────────────────────────────────

static bool _joySelPrev  = false;
static char _joyPosPrev  = JOYSTICK_NONE;

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
            if (joyRight || joyEdge) { DisplayTimeoutReset(); UIPageNext(); }
            if (joyLeft)             { DisplayTimeoutReset(); UIPagePrev(); }
            if ((millis() - LastDisplayUpdate) > 3000) {
                LastDisplayUpdate = millis();
                UIPageDraw();
            }
        } else {
            if (joyEdge) DisplayTimeoutReset();
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

// ── WiFi connect animation ────────────────────────────────────────────────────

static char _wifiAnimK = 0;

void DisplayWiFiConnect() {
    DisplaycurrentMillis = millis();
    if (DisplaycurrentMillis - DisplayRefreshRate >= 250) {
        DisplayRefreshRate = DisplaycurrentMillis;
        if (++_wifiAnimK > 3) _wifiAnimK = 0;
        _hw_wifi_signal(_wifiAnimK);
    }
}

// ── WiFi RSSI indicator ───────────────────────────────────────────────────────

char LastWiFiSig = 0;

void WiFiCheckRSSI(char overide) {
    long rssi = WiFi.RSSI() * -1;
    uint8_t lvl = (rssi < HIGHRSSI) ? 1 : (rssi < LOWRSSI) ? 2 : 3;
    if ((LastWiFiSig != (char)lvl) || overide) {
        _hw_wifi_signal(lvl);
        LastWiFiSig = (char)lvl;
    }
}

// ── MQTT status ───────────────────────────────────────────────────────────────

char LastMQTT = 0;

void CheckMQTTCon(char overide) {
    if (GetMQTTStatus() == 1) {
        if ((LastMQTT != 1) || (overide == 1)) { DisplayMQTT(1); LastMQTT = 1; }
    } else {
        if ((LastMQTT != 2) || (overide == 1)) { DisplayMQTT(0); LastMQTT = 2; }
    }
}

// ── Center-screen rotation ────────────────────────────────────────────────────

void DisplaySwitchCase() {
    switch (ScreenShow) {
        case 0: DisplayCenterInput();  ScreenShow++; break;
        case 1: DisplayCenterOutput(); ScreenShow++; break;
        case 2: DisplayCenterIPInfo(); ScreenShow++; break;
        default: ScreenShow = 0; break;
    }
    Log(DEBUG, "Screen = %d\r\n", (int)ScreenShow);
}

// ── Thin dispatch wrappers ────────────────────────────────────────────────────

void DisplayWiFiRSSI()             { WiFiCheckRSSI(1); }
void DisplayTHBar()                { _hw_th_bar(); }
void DisplayID()                   { _hw_id_label(); }
void DisplayMQTT(char mode)        { _hw_mqtt_icon((uint8_t)mode); }
void DisplayClear()                { _hw_clear(); }
void DisplayLogo()                 { _hw_logo(); }
void DisplayLog(const char* t)     { _hw_boot_log(t); }
void DisplayAPInfo(const char* s)  { _hw_ap_info(s); }
void DisplayCenterInput()          { _hw_center_input(); }
void DisplayCenterOutput()         { _hw_center_output(); }
void DisplayCenterIPInfo()         { _hw_center_ip(); }
void DisplayCenterRemoteInfo()     { _hw_center_remote(); }
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
