
#include "TFT.h"
#include "HAL/Digital/Digital.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <SPI.h>
#include <esp_heap_caps.h>
#include <qrcode.h>
#include "Functions.h"
#include <PNGdec.h>
#include <new>
#include "Sensors.h"
#include "TFTBitMaps.h"
#include "Devices/Log.h"
#include "Define.h"
#include "WifiControl/WifiConfig.h"
#include "FileSystem/FSInterface.h"

#define MAX_IMAGE_WIDTH 240

// PNG decoder is ~45 KB — only needed to draw the boot logo once, so it lives
// in PSRAM for the duration of TFTLogo() instead of permanently in internal RAM.
static PNG* _png = nullptr;

TFT_eSPI tft = TFT_eSPI(240, 320);
TFT_eSprite Screen    = TFT_eSprite(&tft);  // full-screen back buffer, drawn into PSRAM

static int16_t xpos = 40;
static int16_t ypos = 60;

int pngDraw(PNGDRAW *pDraw);

// Boot log — lines shift up as new ones arrive, oldest fade out
#define BOOT_LOG_LINES  10
#define BOOT_LOG_LINE_H 11          // 8px text + 3px gap
#define BOOT_LOG_Y      120         // below 180x108 logo drawn at y=5

static char    (*_bootBuf)[52] = nullptr;
static uint8_t _bootN = 0;

static void _bootDrawLine(uint8_t row, const char* text) {
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(21, BOOT_LOG_Y + row * BOOT_LOG_LINE_H);
    tft.print("> ");
    tft.print(text);
}

void TFTBootLog(const char* line) {
    if (!_bootBuf) return;
    if (_bootN < BOOT_LOG_LINES) {
        // Append: draw only the new line, never touch previous ones
        uint8_t row = _bootN;
        strlcpy(_bootBuf[_bootN++], line, sizeof(_bootBuf[0]));
        _bootDrawLine(row, _bootBuf[row]);
    } else {
        // Scroll (>10 lines — rare): shift buffer, clear area, redraw all
        for (uint8_t i = 0; i < BOOT_LOG_LINES - 1; i++)
            strlcpy(_bootBuf[i], _bootBuf[i + 1], sizeof(_bootBuf[0]));
        strlcpy(_bootBuf[BOOT_LOG_LINES - 1], line, sizeof(_bootBuf[0]));
        tft.fillRect(0, BOOT_LOG_Y, 320, BOOT_LOG_LINES * BOOT_LOG_LINE_H + 2, TFT_BLACK);
        for (uint8_t i = 0; i < BOOT_LOG_LINES; i++)
            _bootDrawLine(i, _bootBuf[i]);
    }
}

void TFTInit() {
    tft.init();
    tft.setRotation(3);
    tft.invertDisplay(1);
    tft.fillScreen(TFT_BLACK);

    // Full-screen back buffer — uses PSRAM (150 KB) when BOARD_HAS_PSRAM is set,
    // falls back to internal SRAM if ps_malloc returns null.
    Screen.setColorDepth(16);
    void* buf = Screen.createSprite(320, 240);  // landscape: 320 wide × 240 tall
    if (buf)
        Log(NOTIFY, "TFT: Screen sprite OK (%s)\r\n",
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0 ? "PSRAM" : "SRAM");
    else
        Log(ERROR, "TFT: Screen sprite alloc failed — drawing direct\r\n");

    _bootBuf = (char(*)[52]) heap_caps_malloc(BOOT_LOG_LINES * 52, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_bootBuf) _bootBuf = (char(*)[52]) malloc(BOOT_LOG_LINES * 52);
    // Logo centered horizontally (200px wide) at top for boot screen
    xpos = (320 - 200) / 2;  // = 60
    ypos = 5;
    TFTLogo();
}

void TFTLogo() {
    void* mem = heap_caps_malloc(sizeof(PNG), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!mem) mem = malloc(sizeof(PNG));
    if (!mem) return;                       // no logo — not worth failing boot over
    _png = new (mem) PNG();
    if (_png->openFLASH((uint8_t *)Logo, sizeof(Logo), pngDraw) == PNG_SUCCESS) {
        tft.startWrite();
        _png->decode(NULL, 0);
        tft.endWrite();
    }
    _png->~PNG();
    free(mem);
    _png = nullptr;
}

int pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    _png->getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xff000000);
    tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
    return 1;
}

void TFTDisplayClear() {
    tft.fillScreen(TFT_BLACK);
}

void TFTDisplayAPInfo(const char* ssid) {
    tft.fillScreen(TFT_BLACK);

    // ── Header ──────────────────────────────────────────────────────────────
    tft.fillRect(0, 0, 320, 26, TFT_NAVY);
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.setCursor(IsForcedAPMode() ? 30 : 38, 5);
    tft.print(IsForcedAPMode() ? "FORCED AP MODE" : "AP MODE");

    // ── WiFi network ────────────────────────────────────────────────────────
    tft.setTextColor(0x4208); // dim grey label
    tft.setTextSize(1);
    tft.setCursor(22, 33);
    tft.print("WiFi Network");

    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(22, 44);
    tft.print(ssid);                         // e.g. ESPPLC-18152

    // thin separator
    tft.drawFastHLine(22, 67, 160, 0x4208);

    // ── Browser URL ─────────────────────────────────────────────────────────
    tft.setTextColor(0x4208);
    tft.setTextSize(1);
    tft.setCursor(22, 72);
    tft.print("Open Browser");

    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.setCursor(22, 83);
    tft.print("192.168.4.1");

    // mDNS alternative — just hostname.local, no http:// prefix
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(1);
    tft.setCursor(22, 106);
    char mdns[48];
    snprintf(mdns, sizeof(mdns), "%s.local", ssid);
    tft.print(mdns);

    // ── Password ─────────────────────────────────────────────────────────────
    String apPass = GetAPPassword();
    tft.setTextColor(0x4208);
    tft.setTextSize(1);
    tft.setCursor(22, 118);
    tft.print("Password");
    tft.setTextSize(2);
    tft.setCursor(22, 128);
    if (apPass.length() == 0) {
        // Open AP after a boot-button reset — no password, no QR code
        tft.setTextColor(TFT_YELLOW);
        tft.print("OPEN");
        tft.setTextSize(1);
        tft.setCursor(190, 40);
        tft.print("Button reset:");
        tft.setCursor(190, 52);
        tft.print("open network.");
        tft.setCursor(190, 70);
        tft.print("Save WiFi on the");
        tft.setCursor(190, 82);
        tft.print("web page to");
        tft.setCursor(190, 94);
        tft.print("secure it.");
        return;
    }
    tft.setTextColor(TFT_WHITE);
    tft.print(apPass);

    // ── QR code — version 3 (29 modules), scale 3 = 87x87 px ───────────────
    QRCode qrcode;
    char qrText[96];
    snprintf(qrText, sizeof(qrText), "WIFI:T:WPA;S:%s;P:%s;;", ssid, apPass.c_str());
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrText);

    const int qrScale = 3;
    const int qrX     = 190;
    const int qrY     = 32;
    tft.fillRect(qrX - 4, qrY - 4,
                 qrcode.size * qrScale + 8,
                 qrcode.size * qrScale + 8, TFT_WHITE);
    tft.startWrite();
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            uint16_t col = qrcode_getModule(&qrcode, x, y) ? TFT_BLACK : TFT_WHITE;
            tft.fillRect(qrX + x * qrScale, qrY + y * qrScale, qrScale, qrScale, col);
        }
    }
    tft.endWrite();

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(qrX - 4, qrY + qrcode.size * qrScale + 10);
    tft.print("Scan to join WiFi");
}

// ── Display hardware seam (Display.cpp calls these) ───────────────────────────
#ifdef DISPLAY_TFT
#include "Display.h"

void _hw_init() {
    Log(NOTIFY, "TFT Init");
    TFTInit();
    ledcSetup(0, LEDC_BASE_FREQ, 12);
    ledcAttachPin(LED_PIN, 0);
}

void _hw_clear()                    { TFTDisplayClear(); }
void _hw_brightness(uint8_t b)      { ledcWrite(0, (4095 / 255) * b); }
void _hw_boot_log(const char* line) { TFTBootLog(line); }
void _hw_ap_info(const char* ssid)  { TFTDisplayAPInfo(ssid); }
#endif // DISPLAY_TFT
