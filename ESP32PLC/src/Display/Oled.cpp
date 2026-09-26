#ifdef DISPLAY_OLED
// Minimal SSD1306 driver behind the Display.cpp hardware seam. The page UI
// (UIPages) is TFT-only; on OLED hardware this shows a splash and clears.
#include "Oled.h"
#include "OledBitMaps.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Devices/Log.h"

#define SCREEN_ADDRESS 0x3C   // 0x3D for 128x64 on some modules

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool _ok = false;

void OLEDInit(void) {
    _ok = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    if (!_ok) { Log(ERROR, "OLED: SSD1306 init failed"); return; }
    display.clearDisplay();
    display.drawBitmap((display.width() - 32) / 2, (display.height() - 32) / 2, Home, 32, 32, 1);
    display.display();
}

void OledDisplayClear(void) {
    if (!_ok) return;
    display.clearDisplay();
    display.display();
}

// ── Display hardware seam (Display.cpp calls these) ───────────────────────────
void _hw_init()                     { Log(NOTIFY, "OLED Init"); OLEDInit(); }
void _hw_clear()                    { OledDisplayClear(); }
void _hw_brightness(uint8_t)        {}  // no PWM backlight on OLED
void _hw_boot_log(const char*)      {}
void _hw_ap_info(const char*)       {}
#endif // DISPLAY_OLED
