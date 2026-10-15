#include "ControlTask.h"
#include <Arduino.h>
#include "HAL/Digital/Digital.h"
#include "Remote/MasterController.h"
#include "Remote/FwUpdater.h"
#include "Modules/ModuleManager.h"
#include "Devices/Log.h"

// ── Control task ─────────────────────────────────────────────────────────────
// The time-critical work — digital inputs, Modbus/RS-485 polling and writes, and
// realtime modules (fireworks sequencing) — runs here instead of in loop().
//
// loop() still does WiFi recovery, MQTT, the display, sensors and OTA, any of
// which can stall: an MQTT connect to a dead broker blocks for seconds, a display
// redraw takes ~30-40 ms. This task sits on the same core as loop() (core 1; the
// WiFi stack has core 0) at a HIGHER priority, so it preempts loop() whenever it
// is ready and those stalls can't delay it. Every step here is non-blocking; the
// 1 ms delay each pass hands the core back to loop().
//
// Rules for anything called from here: no MQTT (PubSubClient isn't thread-safe —
// device status messages are queued and published by loop()), no FastLED, no
// display.

#define CONTROL_CORE      1
#define CONTROL_PRIORITY  3      // loop() runs at 1
#define CONTROL_STACK     8192

static uint32_t _maxUs = 0, _peakUs = 0, _count = 0, _winStartMs = 0;
static uint64_t _sumUs = 0;

static void _controlTask(void*) {
    for (;;) {
        uint32_t t0 = micros();

        ScanIO();                                   // inputs (fireworks IN0 trigger, MQTT, web)
        RemoteRun();                                // Modbus polling + queued writes (non-blocking)
        if (!fwUpdater.getStatus().running)         // a device job owns the bus meanwhile
            modules.update(true);                   // realtime modules only

        uint32_t dt = micros() - t0;
        if (dt > _maxUs)  _maxUs  = dt;
        if (dt > _peakUs) _peakUs = dt;
        _sumUs += dt; _count++;

        vTaskDelay(1);                              // 1 ms: let loop() run
    }
}

void StartControlTask() {
    _winStartMs = millis();
    BaseType_t rc = xTaskCreatePinnedToCore(_controlTask, "control", CONTROL_STACK, nullptr,
                                            CONTROL_PRIORITY, nullptr, CONTROL_CORE);
    if (rc == pdPASS) Log(LOG, "Control task started (core %d, prio %d)", CONTROL_CORE, CONTROL_PRIORITY);
    else              Log(ERROR, "Control task FAILED to start");
}

void ControlStatsRead(uint32_t* maxUs, uint32_t* peakUs, uint32_t* avgUs, uint32_t* perSec) {
    uint32_t winMs = millis() - _winStartMs;
    *maxUs  = _maxUs;
    *peakUs = _peakUs;
    *avgUs  = _count ? (uint32_t)(_sumUs / _count) : 0;
    *perSec = winMs ? (uint32_t)((uint64_t)_count * 1000 / winMs) : 0;
    _maxUs = 0; _sumUs = 0; _count = 0; _winStartMs = millis();
}
