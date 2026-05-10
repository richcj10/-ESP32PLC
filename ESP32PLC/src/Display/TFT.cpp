
#include "TFT.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <SPI.h>
#include <esp_heap_caps.h>
#include <qrcode.h>
#include "OledBitMaps.h"
#include "Functions.h"
#include <PNGdec.h>
#include "Sensors.h"
#include "TFTBitMaps.h"
#include "Devices/Log.h"

#define MAX_IMAGE_WIDTH 240

PNG png;

TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite Screen    = TFT_eSprite(&tft);  // full-screen back buffer, drawn into PSRAM
TFT_eSprite StatusBar = TFT_eSprite(&tft);
TFT_eSprite Bottom    = TFT_eSprite(&tft);

static int16_t xpos = 40;
static int16_t ypos = 60;

int pngDraw(PNGDRAW *pDraw);

// Boot log — lines shift up as new ones arrive, oldest fade out
#define BOOT_LOG_LINES  10
#define BOOT_LOG_LINE_H 11          // 8px text + 3px gap
#define BOOT_LOG_Y      120         // below 180x108 logo drawn at y=5

static char    (*_bootBuf)[52] = nullptr;
static uint8_t _bootN = 0;

void TFTBootLog(const char* line) {
    if (!_bootBuf) return;
    if (_bootN < BOOT_LOG_LINES) {
        strlcpy(_bootBuf[_bootN++], line, sizeof(_bootBuf[0]));
    } else {
        for (uint8_t i = 0; i < BOOT_LOG_LINES - 1; i++)
            strlcpy(_bootBuf[i], _bootBuf[i + 1], sizeof(_bootBuf[0]));
        strlcpy(_bootBuf[BOOT_LOG_LINES - 1], line, sizeof(_bootBuf[0]));
    }
    tft.fillRect(0, BOOT_LOG_Y, 320, BOOT_LOG_LINES * BOOT_LOG_LINE_H + 2, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    for (uint8_t i = 0; i < _bootN; i++) {
        int age = (_bootN - 1) - i;   // 0 = newest (bottom)
        uint16_t col = (age == 0) ? TFT_WHITE
                     : (age <  3) ? 0xC618   // light grey
                                  : 0x4208;  // dim grey
        tft.setTextColor(col);
        tft.setCursor(6, BOOT_LOG_Y + i * BOOT_LOG_LINE_H);
        tft.print("> ");
        tft.print(_bootBuf[i]);
    }
}

void TFTInit() {
    tft.init();
    tft.invertDisplay(1);
    tft.fillScreen(TFT_BLACK);

    // Full-screen back buffer — uses PSRAM (150 KB) when BOARD_HAS_PSRAM is set,
    // falls back to internal SRAM if ps_malloc returns null.
    Screen.setColorDepth(16);
    void* buf = Screen.createSprite(320, 240);
    if (buf)
        Log(NOTIFY, "TFT: Screen sprite OK (%s)\r\n",
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0 ? "PSRAM" : "SRAM");
    else
        Log(ERROR, "TFT: Screen sprite alloc failed — drawing direct\r\n");

    _bootBuf = (char(*)[52]) heap_caps_malloc(BOOT_LOG_LINES * 52, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_bootBuf) _bootBuf = (char(*)[52]) malloc(BOOT_LOG_LINES * 52);

    StatusBar.createSprite(20, 240);
    Bottom.createSprite(30, 240);
    // Logo centered horizontally (180px wide) at top for boot screen
    xpos = (320 - 180) / 2;  // = 70
    ypos = 5;
    TFTLogo();
}

char TFTLastWiFiSig = 0;

void TFTTHBar() {
    Bottom.setTextSize(2);
    Bottom.setTextColor(TFT_WHITE);
    Bottom.setCursor(8, 5);
    Bottom.println("T: ");
    Bottom.setCursor(8, 55);
    Bottom.println(String(getDeviceClimateTemprature(), 1));
    Bottom.setCursor(8, 108);
    Bottom.println("H: ");
    Bottom.setCursor(8, 140);
    Bottom.println(String(getDeviceClimateHumidity(), 1));
    Bottom.pushSprite(40, TFTBANNERY);
}

void TFTIDSet() {
    StatusBar.setTextSize(1);
    StatusBar.setCursor(100, 8);
    StatusBar.setTextColor(TFT_WHITE);
    StatusBar.print(GetClientId().c_str());
    StatusBar.pushSprite(40, 8);
}

void TFTMQTTIconSet(char IconMode) {
    StatusBar.drawBitmap(20, 8, Clear, 16, 16, TFT_BLACK);
    if (IconMode == 1)
        StatusBar.drawBitmap(20, 8, Connected,    16, 16, TFT_WHITE);
    else
        StatusBar.drawBitmap(20, 8, NotConnected, 16, 16, TFT_WHITE);
    StatusBar.pushSprite(40, 8);
}

void TFTWiFiConnect(char Position) {
    StatusBar.drawBitmap(0, 8, Clear, 16, 16, TFT_BLACK);
    if      (Position == 1) StatusBar.drawBitmap(0, 8, SignalSmall, 16, 16, TFT_RED);
    else if (Position == 2) StatusBar.drawBitmap(0, 8, SignalMed,   16, 16, TFT_YELLOW);
    else if (Position == 3) StatusBar.drawBitmap(0, 8, Sig,         16, 16, TFT_GREEN);
    StatusBar.pushSprite(40, 8);
}

void TFTDisplayInputs() {
    TFTCenterClear();
    tft.setTextSize(2);
    tft.setCursor(30, 20);
    tft.print("Inputs:");
    tft.drawCircle(30, 60, 4, TFT_WHITE);
    tft.drawCircle(50, 60, 4, TFT_WHITE);
    tft.drawCircle(70, 60, 4, TFT_WHITE);
    tft.drawCircle(90, 60, 4, TFT_WHITE);
}

void TFTDisplayOutputs() {
    TFTCenterClear();
    tft.setTextSize(2);
    tft.setCursor(30, 20);
    tft.print("Outputs:");
    tft.drawCircle(30, 60, 4, TFT_WHITE);
    tft.drawCircle(50, 60, 4, TFT_WHITE);
    tft.drawCircle(70, 60, 4, TFT_WHITE);
    tft.drawCircle(90, 60, 4, TFT_WHITE);
}

void TFTDisplayRemote() {
    TFTCenterClear();
    tft.setTextSize(2);
    tft.setCursor(60, 50);
    tft.setTextColor(TFT_WHITE);
    tft.print("Remote:");
}

void TFTDisplayIP() {
    TFTCenterClear();
    tft.setTextSize(1);
    tft.setCursor(60, 50);
    tft.setTextColor(TFT_WHITE);
    tft.print("IP: 0.0.0.0");
    tft.setCursor(60, 70);
    tft.print("MQTT: 0.0.0.0");
}

void TFTLogoDisplay() { TFTLogo(); }

void TFTLogo() {
    int16_t rc = png.openFLASH((uint8_t *)Logo, sizeof(Logo), pngDraw);
    if (rc == PNG_SUCCESS) {
        tft.startWrite();
        png.decode(NULL, 0);
        tft.endWrite();
    }
}

int pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
    return 1;
}

void TFTLog(const char *Comment) {
    tft.fillRect(INFOX, INFOY, 600, 15, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(INFOX + 5, INFOY);
    tft.setTextColor(TFT_WHITE);
    tft.print(Comment);
}

char GraphPos = 0;

void TFTBargraph(char Mode) {
    if (Mode == 1) {
        tft.fillRect(35,  86, 5, 16, TFT_RED);
        tft.fillRect(37,  89, 3, 10, TFT_WHITE);
        tft.fillRect(210, 86, 5, 16, TFT_RED);
        tft.fillRect(216, 89, 3, 13, TFT_WHITE);
        GraphPos = 0;
    }
    if (Mode == 2) {
        tft.fillRect(5, 20, 200, 50, TFT_BLACK);
    }
}

void TFTBargraphUpdate(unsigned int Precent) {
    GraphPos = Precent / 5;
    int Locx = 0;
    tft.startWrite();
    for (char l = 0; l < GraphPos; l++) {
        Locx = (10 * l) + 40;
        tft.fillRect(Locx, 90, 3, 13, TFT_WHITE);
    }
    tft.endWrite();
}

void TFTDisplayClear() {
    tft.fillScreen(TFT_BLACK);
}

void TFTBarClear() {
    tft.fillRect(0, TFTBANNERY, 200, 20, TFT_BLACK);
}

void TFTCenterClear() {
    tft.fillRect(0, 20, 200, 120, TFT_BLACK);
}

void TFTDisplayAPInfo(const char* ssid) {
    tft.fillScreen(TFT_BLACK);

    // ── Header ──────────────────────────────────────────────────────────────
    tft.fillRect(0, 0, 320, 26, TFT_NAVY);
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.setCursor(38, 5);
    tft.print("AP MODE");

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

    // ── Hint ────────────────────────────────────────────────────────────────
    tft.setTextColor(0x4208);
    tft.setCursor(22, 125);
    tft.print("No password  |  Scan QR");

    // ── QR code — version 3 (29 modules), scale 3 = 87x87 px ───────────────
    QRCode qrcode;
    char qrText[72];
    snprintf(qrText, sizeof(qrText), "WIFI:T:nopass;S:%s;;", ssid);
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
    TFTLogo();
}

void _hw_clear()                    { TFTDisplayClear(); }
void _hw_brightness(uint8_t b)      { ledcWrite(0, (4095 / 255) * b); }
void _hw_wifi_signal(uint8_t lvl)   { TFTWiFiConnect((char)lvl); }
void _hw_mqtt_icon(uint8_t mode)    { TFTMQTTIconSet((char)mode); }
void _hw_th_bar()                   { TFTTHBar(); }
void _hw_id_label()                 { TFTIDSet(); }
void _hw_logo()                     { TFTLogoDisplay(); }
void _hw_boot_log(const char* line) { TFTBootLog(line); }
void _hw_ap_info(const char* ssid)  { TFTDisplayAPInfo(ssid); }
void _hw_center_input()             { TFTDisplayInputs(); }
void _hw_center_output()            { TFTDisplayOutputs(); }
void _hw_center_ip()                { TFTDisplayIP(); }
void _hw_center_remote()            { TFTDisplayRemote(); }
#endif // DISPLAY_TFT
