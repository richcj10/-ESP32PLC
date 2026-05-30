#include "UIPages.h"
#include "TFT.h"
#include "Devices/Log.h"
#include <WiFi.h>
#include "Functions.h"
#include "Sensors.h"
#include "HAL/Digital/Digital.h"
#include "Remote/MasterController.h"
#include "MQTT.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"

#define PAGE_COUNT  4

// Uncomment to show pixel-grid test pattern instead of normal pages.
//#define UI_TEST_PATTERN

// Sprite: 320×240 landscape.  Visible through housing bezel: x=20..300, y=0..239
#define DISP_W     320
#define DISP_H     240
#define VIS_LEFT    20    // first visible pixel on left (bezel clips 0..19)
#define VIS_RIGHT  300    // last visible pixel on right (bezel clips 301..319)
#define MARGIN      25    // left content margin — just inside bezel
#define HDR_H       24    // header bar height (cyan line at y=24)
#define HDR_BG    0x2104  // dark gray — header and card background
#define CARD_X      MARGIN                    // card starts at margin
#define CARD_W      270                       // CARD_X(25)+CARD_W(270)=295, inside VIS_RIGHT
#define CARD_Y      26                        // card top (below header)
#define CARD_H      185                       // card ends at y=211
#define CONTENT_X  (MARGIN + 6)              // text left margin inside card
#define DOTS_Y      225                       // dots below card, above bottom bezel

static uint8_t _page = 0;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void _wifiIcon(int x, int y) {
    long rssi = WiFi.RSSI();
    uint8_t level = (rssi > -65) ? 3 : (rssi > -80) ? 2 : 1;
    for (int b = 0; b < 3; b++) {
        int bh = 4 + b * 4;
        uint16_t col = ((uint8_t)(b + 1) <= level) ? (uint16_t)TFT_GREEN : (uint16_t)HDR_BG;
        Screen.fillRect(x + b * 5, y + 12 - bh, 3, bh, col);
    }
}

static void _mqttIcon(int x, int y) {
    bool en  = GetMQTTEnabled();
    bool con = GetMQTTStatus() == 1;
    uint16_t col = !en ? (uint16_t)TFT_DARKGREY : (con ? (uint16_t)TFT_GREEN : (uint16_t)TFT_RED);
    Screen.fillCircle(x, y, 4, col);
}

static void _header(const char* title) {
    Screen.fillSprite(TFT_BLACK);
    Screen.fillRect(0, 0, DISP_W, HDR_H, HDR_BG);

    // Uptime (left, past bezel margin)
    unsigned long s = millis() / 1000;
    char ub[10];
    if (s < 3600)
        snprintf(ub, sizeof(ub), "%um%02us", (unsigned)(s / 60), (unsigned)(s % 60));
    else
        snprintf(ub, sizeof(ub), "%uh%02um", (unsigned)(s / 3600), (unsigned)((s % 3600) / 60));
    Screen.setTextSize(1);
    Screen.setTextColor(TFT_DARKGREY, HDR_BG);
    Screen.setCursor(MARGIN, 7);
    Screen.print(ub);

    // Title centered across full display width
    Screen.setTextSize(2);
    Screen.setTextColor(TFT_WHITE, HDR_BG);
    int tw = (int)strlen(title) * 12;
    Screen.setCursor((DISP_W - tw) / 2, 6);
    Screen.print(title);

    // WiFi bars + MQTT dot — keep within VIS_RIGHT=300
    _wifiIcon(VIS_RIGHT - 28, 4);   // bars at 272,277,282
    _mqttIcon(VIS_RIGHT - 6, 11);   // dot at 294 (r=4 → 290..298)

    Screen.drawFastHLine(0, HDR_H, DISP_W, TFT_CYAN);
}

static void _card() {
    Screen.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 5, HDR_BG);
}

static void _dots() {
    const int sp = 16;
    int sx = (DISP_W - (PAGE_COUNT - 1) * sp) / 2;
    for (int i = 0; i < PAGE_COUNT; i++) {
        int dx = sx + i * sp;
        if (i == (int)_page) Screen.fillCircle(dx, DOTS_Y, 4, TFT_CYAN);
        else                 Screen.drawCircle(dx, DOTS_Y, 3, TFT_DARKGREY);
    }
}

// ── Page 0: NETWORK ───────────────────────────────────────────────────────────

static void _drawNetwork() {
    _header("NETWORK");
    _card();
    Screen.setTextSize(1);

    int y = CARD_Y + 10;
    int gap = 36;

    Screen.setCursor(CONTENT_X, y);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("Host: ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);    Screen.print(GetHostName().c_str());
    y += gap;

    Screen.setCursor(CONTENT_X, y);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("IP:   ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);    Screen.print(GetIPStr().c_str());
    y += gap;

    Screen.setCursor(CONTENT_X, y);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("Mode: ");
    Screen.setTextColor(TFT_YELLOW, HDR_BG);
    Screen.print(GetWiFiMode() == WIFI_AP_MODE ? "AP" : "STA");
    y += gap;

    long rssi = WiFi.RSSI();
    uint16_t rssiCol = (rssi > -65) ? TFT_GREEN : (rssi > -80) ? TFT_YELLOW : TFT_RED;
    char rBuf[16];
    snprintf(rBuf, sizeof(rBuf), "%ld dBm", rssi);
    Screen.setCursor(CONTENT_X, y);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("RSSI: ");
    Screen.setTextColor(rssiCol, HDR_BG);      Screen.print(rBuf);
    y += 14;

    int barW = CARD_W - 20;
    int barLen = (int)constrain(map(rssi, -100, -40, 0, barW), 0, barW);
    Screen.fillRect(CONTENT_X, y, barW, 8, 0x2104);
    Screen.fillRect(CONTENT_X, y, barLen, 8, rssiCol);
    y += 20;

    unsigned long tot = millis() / 1000;
    char upBuf[20];
    uint16_t days = (uint16_t)(tot / 86400); tot %= 86400;
    uint8_t  hrs  = (uint8_t) (tot / 3600);  tot %= 3600;
    uint8_t  mins = (uint8_t) (tot / 60);
    uint8_t  secs = (uint8_t) (tot % 60);
    if (days > 0)       snprintf(upBuf, sizeof(upBuf), "%ud %uh %um", days, hrs, mins);
    else if (hrs > 0)   snprintf(upBuf, sizeof(upBuf), "%uh %um %us", hrs, mins, secs);
    else                snprintf(upBuf, sizeof(upBuf), "%um %us", mins, secs);

    Screen.setCursor(CONTENT_X, y);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("Up:   ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);    Screen.print(upBuf);
}

// ── Page 1: I/O ───────────────────────────────────────────────────────────────

static void _drawIO() {
    _header("I / O");
    _card();
    Screen.setTextSize(1);

    Screen.setTextColor(TFT_DARKGREY, HDR_BG);
    Screen.setCursor(CONTENT_X, CARD_Y + 10);
    Screen.print("Fire Channels:");

    // 5 circles evenly spaced across card width
    int circleY = CARD_Y + 75;
    for (int i = 0; i < 5; i++) {
        int cx = CARD_X + 20 + i * ((CARD_W - 20) / 5) + ((CARD_W - 20) / 10);
        bool active = GetCHFire(i + 1) == 1;
        if (active) {
            Screen.fillCircle(cx, circleY, 14, TFT_RED);
            Screen.setTextColor(TFT_WHITE, TFT_RED);
        } else {
            Screen.fillCircle(cx, circleY, 14, HDR_BG);
            Screen.drawCircle(cx, circleY, 14, TFT_DARKGREY);
            Screen.setTextColor(TFT_DARKGREY, HDR_BG);
        }
        char lbl[3];
        snprintf(lbl, sizeof(lbl), "%d", i + 1);
        Screen.setCursor(cx - 3, circleY - 4);
        Screen.print(lbl);
    }

    int y = CARD_Y + 120;
    Screen.setCursor(CONTENT_X, y);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("Temp: ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);
    char tBuf[12];
    snprintf(tBuf, sizeof(tBuf), "%.1fC", getDeviceClimateTemprature());
    Screen.print(tBuf);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("  Hum: ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);
    char hBuf[10];
    snprintf(hBuf, sizeof(hBuf), "%.0f%%", getDeviceClimateHumidity());
    Screen.print(hBuf);
    y += 30;

    Screen.setCursor(CONTENT_X, y);
    bool sw = GetUserSWValue();
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("SW: ");
    Screen.setTextColor(sw ? TFT_GREEN : TFT_RED, HDR_BG);
    Screen.print(sw ? "ON " : "OFF");
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("  Occ: ");
    bool occ = GetOccupied();
    Screen.setTextColor(occ ? TFT_GREEN : TFT_WHITE, HDR_BG);
    Screen.print(occ ? "YES" : "NO ");
}

// ── Page 2: MODBUS ────────────────────────────────────────────────────────────

static void _drawModbus() {
    _header("MODBUS");
    _card();
    Screen.setTextSize(1);

    FwDeviceInfo devs[8];
    uint8_t cnt = GetFwDeviceList(devs, 8);

    if (cnt == 0) {
        Screen.setTextColor(TFT_DARKGREY, HDR_BG);
        Screen.setCursor(CONTENT_X, CARD_Y + 30);
        Screen.print("No devices configured");
        return;
    }

    uint8_t limit = (cnt < 6) ? cnt : 6;
    int rowH = (DOTS_Y - CARD_Y - 14) / limit;
    if (rowH > 34) rowH = 34;

    for (uint8_t i = 0; i < limit; i++) {
        int y = CARD_Y + 10 + (int)i * rowH;
        ModbusDevice* dev = master.findByAddress(devs[i].slaveId);
        bool ok = dev && dev->isResponding();

        Screen.fillCircle(CONTENT_X + 4, y + 7, 5, ok ? (uint16_t)TFT_GREEN : (uint16_t)TFT_RED);

        Screen.setTextColor(TFT_WHITE, HDR_BG);
        Screen.setCursor(CONTENT_X + 16, y);
        Screen.print(devs[i].name);

        char info[24];
        snprintf(info, sizeof(info), "0x%02X  %s", devs[i].slaveId, ok ? "ONLINE" : "OFFLINE");
        Screen.setTextColor(ok ? TFT_GREEN : TFT_RED, HDR_BG);
        Screen.setCursor(CONTENT_X + 16, y + 14);
        Screen.print(info);

        if (dev && dev->totalRequests() > 0) {
            char rBuf[16];
            snprintf(rBuf, sizeof(rBuf), "%lu/%lu",
                     (unsigned long)dev->successRequests(),
                     (unsigned long)dev->totalRequests());
            Screen.setTextColor(TFT_DARKGREY, HDR_BG);
            Screen.setCursor(CARD_X + CARD_W - 48, y + 14);
            Screen.print(rBuf);
        }
    }
}

// ── Page 3: MQTT ──────────────────────────────────────────────────────────────

static void _drawMQTT() {
    _header("MQTT");
    _card();

    bool connected = GetMQTTStatus() == 1;
    bool enabled   = GetMQTTEnabled();

    const char* status = !enabled ? "DISABLED" : (connected ? "CONNECTED" : "OFFLINE");
    uint16_t    col    = !enabled ? (uint16_t)TFT_DARKGREY
                                  : (connected ? (uint16_t)TFT_GREEN : (uint16_t)TFT_RED);

    Screen.setTextSize(2);
    Screen.setTextColor(col, HDR_BG);
    int sw = (int)strlen(status) * 12;
    Screen.setCursor((DISP_W - sw) / 2, CARD_Y + 40);
    Screen.print(status);

    Screen.setTextSize(1);
    char broker[52];
    snprintf(broker, sizeof(broker), "%s:%u",
             GetMQTTIP().c_str(), (unsigned)GetMQTTPort());
    Screen.setCursor(CONTENT_X, CARD_Y + 120);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("Broker: ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);    Screen.print(broker);

    Screen.setCursor(CONTENT_X, CARD_Y + 155);
    Screen.setTextColor(TFT_CYAN, HDR_BG); Screen.print("User:   ");
    Screen.setTextColor(TFT_WHITE, HDR_BG);    Screen.print(GetMQTTUser().c_str());
}

// ── Screen test pattern ───────────────────────────────────────────────────────
// Draws a labeled pixel grid across the full sprite so the housing bezel clip
// boundaries can be read directly off the display.
//
// Grid lines every 20 px.  Colour key:
//   RED     — x=0, x=239, y=0, y=319  (sprite hard edges)
//   MAGENTA — x=MARGIN (65)  current left-content boundary
//   CYAN    — x=120, y=160   centre crosshair
//   GREEN   — every 80 px multiple
//   dim     — every other 20 px line
//
// White 6×6 corner squares mark exact sprite corners.
// Numbers label every line at the centre of the display.

void UIPageTestPattern() {
    const uint16_t BG       = 0x2104;
    const uint16_t DIM      = 0x528A;

    Screen.fillSprite(BG);

    // ── Vertical lines (x) — 0..DISP_W-1 ────────────────────────────────────
    Screen.setTextSize(1);
    for (int x = 0; x < DISP_W; x += 20) {
        uint16_t col;
        if (x == 0)                        col = TFT_RED;
        else if (x == DISP_W / 2)         col = TFT_CYAN;
        else if (x % 80 == 0)             col = TFT_GREEN;
        else                              col = DIM;
        Screen.drawFastVLine(x, 0, DISP_H, col);
        char buf[5];
        snprintf(buf, sizeof(buf), "%d", x);
        Screen.setTextColor(col, BG);
        Screen.setCursor(x + 1, DISP_H / 2 + 10);
        Screen.print(buf);
    }
    // Hard right edge
    Screen.drawFastVLine(DISP_W - 1, 0, DISP_H, TFT_RED);
    Screen.setTextColor(TFT_RED, BG);
    Screen.setCursor(DISP_W - 22, DISP_H / 2 + 10);
    Screen.print(DISP_W - 1);

    // MARGIN line — shows current left content boundary
    Screen.drawFastVLine(MARGIN, 0, DISP_H, TFT_MAGENTA);
    Screen.setTextColor(TFT_MAGENTA, BG);
    Screen.setCursor(MARGIN + 1, 5);
    Screen.print("M=");
    Screen.print(MARGIN);

    // ── Horizontal lines (y) — 0..DISP_H-1 ──────────────────────────────────
    for (int y = 0; y < DISP_H; y += 20) {
        uint16_t col;
        if (y == 0)                        col = TFT_RED;
        else if (y == DISP_H / 2)         col = TFT_CYAN;
        else if (y % 80 == 0)             col = TFT_GREEN;
        else                              col = DIM;
        Screen.drawFastHLine(0, y, DISP_W, col);
        char buf[5];
        snprintf(buf, sizeof(buf), "%d", y);
        Screen.setTextColor(col, BG);
        Screen.setCursor(DISP_W / 2 + 5, y + 1);
        Screen.print(buf);
    }
    // Hard bottom edge
    Screen.drawFastHLine(0, DISP_H - 1, DISP_W, TFT_RED);
    Screen.setTextColor(TFT_RED, BG);
    Screen.setCursor(DISP_W / 2 + 5, DISP_H - 10);
    Screen.print(DISP_H - 1);

    // ── Corner markers ───────────────────────────────────────────────────────
    Screen.fillRect(0,          0,          6, 6, TFT_WHITE);
    Screen.fillRect(DISP_W - 6, 0,          6, 6, TFT_WHITE);
    Screen.fillRect(0,          DISP_H - 6, 6, 6, TFT_WHITE);
    Screen.fillRect(DISP_W - 6, DISP_H - 6, 6, 6, TFT_WHITE);

    // ── Centre crosshair ─────────────────────────────────────────────────────
    Screen.drawFastHLine(DISP_W / 2 - 12, DISP_H / 2, 24, TFT_CYAN);
    Screen.drawFastVLine(DISP_W / 2,      DISP_H / 2 - 12, 24, TFT_CYAN);

    // ── Title ────────────────────────────────────────────────────────────────
    Screen.setTextSize(1);
    Screen.setTextColor(TFT_WHITE, BG);
    Screen.setCursor(DISP_W / 2 - 30, DISP_H / 2 - 10);
    Screen.print("PIXEL GRID");

    Screen.pushSprite(0, 0);
}

// ── Upload status pages ───────────────────────────────────────────────────────

void UIPageUpload(const char* title, uint8_t pct, const char* msg) {
    _header(title ? title : "UPLOAD");
    _card();

    /* Percentage — large, centred */
    char pctStr[6];
    snprintf(pctStr, sizeof(pctStr), "%u%%", (unsigned)pct);
    Screen.setTextSize(3);
    Screen.setTextColor(TFT_CYAN, HDR_BG);
    int tw = (int)strlen(pctStr) * 18;
    Screen.setCursor((DISP_W - tw) / 2, CARD_Y + 20);
    Screen.print(pctStr);

    /* Progress bar */
    const int barX = CARD_X + 12;
    const int barW = CARD_W - 24;
    const int barY = CARD_Y + 78;
    const int barH = 20;
    Screen.fillRect(barX, barY, barW, barH, 0x2104);
    Screen.drawRect(barX - 1, barY - 1, barW + 2, barH + 2, TFT_DARKGREY);
    int filled = (int)((float)barW * (float)pct / 100.0f);
    if (filled > 0)
        Screen.fillRect(barX, barY, filled, barH, TFT_CYAN);

    /* Message */
    if (msg && msg[0]) {
        Screen.setTextSize(1);
        Screen.setTextColor(TFT_WHITE, HDR_BG);
        int mw = (int)strlen(msg) * 6;
        Screen.setCursor((DISP_W - mw) / 2, CARD_Y + 120);
        Screen.print(msg);
    }

    Screen.drawRoundRect(VIS_LEFT,     2, VIS_RIGHT - VIS_LEFT,     DISP_H - 4, 18, TFT_WHITE);
    Screen.drawRoundRect(VIS_LEFT + 1, 3, VIS_RIGHT - VIS_LEFT - 2, DISP_H - 6, 17, TFT_WHITE);
    Screen.pushSprite(0, 0);
}

void UIPageUploadDone(bool success, const char* msg) {
    _header(success ? "COMPLETE" : "UPLOAD FAILED");
    _card();

    uint16_t col       = success ? (uint16_t)TFT_GREEN : (uint16_t)TFT_RED;
    const char* status = success ? "SUCCESS" : "FAILED";

    Screen.setTextSize(3);
    Screen.setTextColor(col, HDR_BG);
    int sw = (int)strlen(status) * 18;
    Screen.setCursor((DISP_W - sw) / 2, CARD_Y + 30);
    Screen.print(status);

    if (msg && msg[0]) {
        Screen.setTextSize(1);
        Screen.setTextColor(TFT_WHITE, HDR_BG);
        int mw = (int)strlen(msg) * 6;
        int mx = (DISP_W - mw) / 2;
        if (mx < CARD_X + 4) mx = CARD_X + 4;
        Screen.setCursor(mx, CARD_Y + 100);
        Screen.print(msg);
    }

    Screen.drawRoundRect(VIS_LEFT,     2, VIS_RIGHT - VIS_LEFT,     DISP_H - 4, 18, TFT_WHITE);
    Screen.drawRoundRect(VIS_LEFT + 1, 3, VIS_RIGHT - VIS_LEFT - 2, DISP_H - 6, 17, TFT_WHITE);
    Screen.pushSprite(0, 0);
}

// ── Public API ────────────────────────────────────────────────────────────────

void UIPageInit() {
    _page = 0;
}

void UIPageDraw() {
    Log(NOTIFY_FORCE, "UIPageDraw: page=%d\r\n", (int)_page);
#ifdef UI_TEST_PATTERN
    UIPageTestPattern();
    return;
#endif
    switch (_page) {
        case 0: _drawNetwork(); break;
        case 1: _drawIO();      break;
        case 2: _drawModbus();  break;
        case 3: _drawMQTT();    break;
    }
    _dots();
    Screen.drawRoundRect(VIS_LEFT,     2, VIS_RIGHT - VIS_LEFT,     DISP_H - 4, 18, TFT_WHITE);
    Screen.drawRoundRect(VIS_LEFT + 1, 3, VIS_RIGHT - VIS_LEFT - 2, DISP_H - 6, 17, TFT_WHITE);
    Log(NOTIFY_FORCE, "UIPageDraw: pushSprite\r\n");
    Screen.pushSprite(0, 0);
    Log(NOTIFY_FORCE, "UIPageDraw: done\r\n");
}

void UIPageNext() {
    _page = (_page + 1) % PAGE_COUNT;
    UIPageDraw();
}

void UIPagePrev() {
    _page = (_page == 0) ? PAGE_COUNT - 1 : _page - 1;
    UIPageDraw();
}
