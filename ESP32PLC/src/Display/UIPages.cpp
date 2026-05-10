#include "UIPages.h"
#include "TFT.h"        // provides extern TFT_eSprite Screen
#include <WiFi.h>
#include "Functions.h"
#include "Sensors.h"
#include "HAL/Digital/Digital.h"
#include "Remote/MasterController.h"
#include "MQTT.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"

#define PAGE_COUNT 4
static uint8_t _page = 0;

// ── Shared header ─────────────────────────────────────────────────────────────

static void _header(const char* title) {
    Screen.fillSprite(TFT_BLACK);
    Screen.setTextSize(2);
    Screen.setTextColor(TFT_CYAN, TFT_BLACK);
    Screen.setCursor(6, 4);
    Screen.print(title);

    char buf[8];
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)_page + 1, PAGE_COUNT);
    Screen.setTextSize(1);
    Screen.setTextColor(0x4208, TFT_BLACK);
    Screen.setCursor(290, 8);
    Screen.print(buf);

    Screen.drawFastHLine(0, 24, 320, TFT_CYAN);
}

// ── Page 0: NETWORK ───────────────────────────────────────────────────────────

static void _drawNetwork() {
    _header("NETWORK");
    Screen.setTextSize(1);
    int y = 32;

    Screen.setCursor(6, y);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("Host: ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK); Screen.print(GetHostName().c_str());
    y += 16;

    Screen.setCursor(6, y);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("IP:   ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK); Screen.print(GetIPStr().c_str());
    y += 16;

    Screen.setCursor(6, y);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("Mode: ");
    Screen.setTextColor(TFT_YELLOW, TFT_BLACK);
    Screen.print(GetWiFiMode() == WIFI_AP_MODE ? "AP" : "STA");
    y += 16;

    long rssi = WiFi.RSSI();
    uint16_t rssiCol = (rssi > -65) ? TFT_GREEN : (rssi > -80) ? TFT_YELLOW : TFT_RED;
    char rBuf[16];
    snprintf(rBuf, sizeof(rBuf), "%ld dBm", rssi);
    Screen.setCursor(6, y);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("RSSI: ");
    Screen.setTextColor(rssiCol, TFT_BLACK); Screen.print(rBuf);
    y += 12;

    int barLen = (int)constrain(map(rssi, -100, -40, 0, 120), 0, 120);
    Screen.fillRect(6, y, 120, 6, 0x2104);
    Screen.fillRect(6, y, barLen, 6, rssiCol);
    y += 16;

    unsigned long tot = millis() / 1000;
    uint16_t days = (uint16_t)(tot / 86400); tot %= 86400;
    uint8_t  hrs  = (uint8_t) (tot / 3600);  tot %= 3600;
    uint8_t  mins = (uint8_t) (tot / 60);
    uint8_t  secs = (uint8_t) (tot % 60);
    char upBuf[28];
    if (days > 0)
        snprintf(upBuf, sizeof(upBuf), "%ud %uh %um", days, hrs, mins);
    else if (hrs > 0)
        snprintf(upBuf, sizeof(upBuf), "%uh %um %us", hrs, mins, secs);
    else
        snprintf(upBuf, sizeof(upBuf), "%um %us", mins, secs);

    Screen.setCursor(6, y);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("Up:   ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK); Screen.print(upBuf);
}

// ── Page 1: I/O ───────────────────────────────────────────────────────────────

static void _drawIO() {
    _header("I / O");
    Screen.setTextSize(1);

    Screen.setTextColor(0x4208, TFT_BLACK);
    Screen.setCursor(6, 32);
    Screen.print("Fire Channels:");

    // Five circles centered across 320px width: cx = 72 + i*44
    for (int i = 0; i < 5; i++) {
        int cx = 72 + i * 44;
        int cy = 60;
        bool active = GetCHFire(i + 1) == 1;
        if (active) {
            Screen.fillCircle(cx, cy, 12, TFT_RED);
            Screen.setTextColor(TFT_WHITE);
        } else {
            Screen.drawCircle(cx, cy, 12, 0x4208);
            Screen.setTextColor(0x4208);
        }
        char lbl[3];
        snprintf(lbl, sizeof(lbl), "%d", i + 1);
        Screen.setCursor(cx - 3, cy - 4);
        Screen.print(lbl);
    }

    // Temperature and humidity
    Screen.setCursor(6, 86);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("Temp: ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK);
    char tBuf[12];
    snprintf(tBuf, sizeof(tBuf), "%.1fC", getDeviceClimateTemprature());
    Screen.print(tBuf);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("   Hum: ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK);
    char hBuf[10];
    snprintf(hBuf, sizeof(hBuf), "%.0f%%", getDeviceClimateHumidity());
    Screen.print(hBuf);

    // User switch and occupancy
    Screen.setCursor(6, 104);
    bool sw = GetUserSWValue();
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("SW: ");
    Screen.setTextColor(sw ? TFT_GREEN : TFT_RED, TFT_BLACK);
    Screen.print(sw ? "ON " : "OFF");
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("   Occ: ");
    bool occ = GetOccupied();
    Screen.setTextColor(occ ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
    Screen.print(occ ? "YES" : "NO ");
}

// ── Page 2: MODBUS ────────────────────────────────────────────────────────────

static void _drawModbus() {
    _header("MODBUS");
    Screen.setTextSize(1);

    FwDeviceInfo devs[8];
    uint8_t cnt = GetFwDeviceList(devs, 8);

    if (cnt == 0) {
        Screen.setTextColor(0x4208, TFT_BLACK);
        Screen.setCursor(6, 50);
        Screen.print("No devices configured");
        return;
    }

    uint8_t limit = (cnt < 6) ? cnt : 6;
    for (uint8_t i = 0; i < limit; i++) {
        int y = 32 + (int)i * 30;
        ModbusDevice* dev = master.findByAddress(devs[i].slaveId);
        bool ok = dev && dev->isResponding();

        if (ok) Screen.fillCircle(10, y + 7, 5, TFT_GREEN);
        else    Screen.drawCircle(10, y + 7, 5, TFT_RED);

        Screen.setTextColor(TFT_WHITE, TFT_BLACK);
        Screen.setCursor(22, y);
        Screen.print(devs[i].name);

        char info[24];
        snprintf(info, sizeof(info), "0x%02X  %s",
                 devs[i].slaveId, ok ? "ONLINE" : "OFFLINE");
        Screen.setTextColor(ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
        Screen.setCursor(22, y + 14);
        Screen.print(info);

        if (dev && dev->totalRequests() > 0) {
            char rBuf[16];
            snprintf(rBuf, sizeof(rBuf), "%lu/%lu",
                     (unsigned long)dev->successRequests(),
                     (unsigned long)dev->totalRequests());
            Screen.setTextColor(0x4208, TFT_BLACK);
            Screen.setCursor(210, y + 14);
            Screen.print(rBuf);
        }
    }
}

// ── Page 3: MQTT ──────────────────────────────────────────────────────────────

static void _drawMQTT() {
    _header("MQTT");

    bool connected = GetMQTTStatus() == 1;
    bool enabled   = GetMQTTEnabled();

    const char* status = !enabled ? "DISABLED" : (connected ? "CONNECTED" : "OFFLINE");
    uint16_t    col    = !enabled ? 0x4208u    : (connected ? (uint16_t)TFT_GREEN : (uint16_t)TFT_RED);

    Screen.setTextSize(3);
    Screen.setTextColor(col, TFT_BLACK);
    Screen.setCursor((320 - (int)strlen(status) * 18) / 2, 44);
    Screen.print(status);

    Screen.setTextSize(1);
    char broker[52];
    snprintf(broker, sizeof(broker), "%s:%u",
             GetMQTTIP().c_str(), (unsigned)GetMQTTPort());
    Screen.setCursor(6, 100);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("Broker: ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK); Screen.print(broker);

    Screen.setCursor(6, 116);
    Screen.setTextColor(0x4208, TFT_BLACK); Screen.print("User:   ");
    Screen.setTextColor(TFT_WHITE, TFT_BLACK); Screen.print(GetMQTTUser().c_str());
}

// ── Public API ────────────────────────────────────────────────────────────────

void UIPageInit() {
    _page = 0;
}

void UIPageDraw() {
    switch (_page) {
        case 0: _drawNetwork(); break;
        case 1: _drawIO();      break;
        case 2: _drawModbus();  break;
        case 3: _drawMQTT();    break;
    }
    Screen.pushSprite(0, 0);  // single SPI burst — display never sees intermediate state
}

void UIPageNext() {
    _page = (_page + 1) % PAGE_COUNT;
    UIPageDraw();
}
