#include "BootReset.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "Define.h"
#include "Devices/Log.h"
#include "Devices/StatusLED.h"
#include "FileSystem/FSInterface.h"

// ── Boot-button reset ─────────────────────────────────────────────────────────
// USER_SW (GPIO0) is the ESP32 BOOT strap: holding it at power-on enters the ROM
// download mode, so we can't check it "at power-on". Instead a background task
// watches it for the first BOOT_WINDOW_MS after start-up while normal boot
// continues (no added boot time).
//
//   Press within the window and hold:
//     0-5 s   yellow blink  — arming; release = cancel
//     5-15 s  solid blue    — release now = NETWORK reset (WiFi + MQTT)
//     15 s    solid red     — FULL factory reset (fires without waiting for release)
//   Then the LED flashes the chosen colour 5x and the device restarts in open-AP
//   mode (no password, no QR) so it can be reached without a working LCD.
//
// Serial log mirrors every step for a unit with a dead LCD and LED.

#define BOOT_WINDOW_MS     10000UL
#define HOLD_NETWORK_MS     5000UL
#define HOLD_FACTORY_MS    15000UL
#define POLL_MS               20

static bool _pressed() { return digitalRead(USER_SW) == LOW; }   // active-low button

static void _flash(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < 5; i++) {
        StatusLEDSet(r, g, b); delay(150);
        StatusLEDSet(0, 0, 0); delay(150);
    }
}

static void _doReset(bool full) {
    LittleFS.begin(false);   // no-op if already mounted; needed to delete files
    if (full) {
        Log(NOTIFY_FORCE, "BootReset: FULL factory reset — restarting in open AP mode\r\n");
        _flash(255, 0, 0);
        FactoryReset(true);
    } else {
        Log(NOTIFY_FORCE, "BootReset: NETWORK reset (WiFi + MQTT) — restarting in open AP mode\r\n");
        _flash(0, 0, 255);
        NetworkReset(true);
    }
    delay(200);   // let the log line drain
    ESP.restart();
}

static void _bootResetTask(void*) {
    pinMode(USER_SW, INPUT);
    uint32_t start     = millis();
    uint32_t pressAt   = 0;      // 0 = not pressed
    uint8_t  stage     = 0;      // 0 idle, 1 arming, 2 network armed
    uint32_t lastBlink = 0;
    bool     blinkOn   = false;

    Log(LOG, "BootReset: hold button now for reset (5 s network / 15 s full)\r\n");

    while (true) {
        uint32_t now = millis();
        bool down = _pressed();

        if (!pressAt) {
            if (now - start > BOOT_WINDOW_MS) break;          // window closed, nothing held
            if (down) {
                pressAt = now; stage = 1;
                StatusLEDOverride(true);
                Log(NOTIFY_FORCE, "BootReset: button held — keep holding (5 s network, 15 s full)\r\n");
            }
        } else if (!down) {
            // Released
            if (stage == 2) _doReset(false);                  // does not return
            Log(NOTIFY_FORCE, "BootReset: released early — cancelled\r\n");
            StatusLEDSet(0, 0, 0);
            StatusLEDOverride(false);
            break;
        } else {
            uint32_t held = now - pressAt;
            if (held >= HOLD_FACTORY_MS) {
                StatusLEDSet(255, 0, 0);
                _doReset(true);                               // does not return
            } else if (held >= HOLD_NETWORK_MS) {
                if (stage != 2) {
                    stage = 2;
                    StatusLEDSet(0, 0, 255);
                    Log(NOTIFY_FORCE, "BootReset: release now = NETWORK reset, keep holding to 15 s = FULL reset\r\n");
                }
            } else if (now - lastBlink >= 125) {              // arming: fast yellow blink
                lastBlink = now; blinkOn = !blinkOn;
                StatusLEDSet(blinkOn ? 255 : 0, blinkOn ? 160 : 0, 0);
            }
        }
        delay(POLL_MS);
    }
    vTaskDelete(nullptr);
}

void BootResetStart() {
    xTaskCreate(_bootResetTask, "bootRst", 4096, nullptr, 1, nullptr);
}
