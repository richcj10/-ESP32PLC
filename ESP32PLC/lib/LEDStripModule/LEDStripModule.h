#pragma once
#include "Modules/Module.h"
#include "Devices/LEDStrip.h"

class LEDStripModule : public Module {
public:
    const char* name() override { return "ledstrip"; }

    void begin()           override { ledStrip.begin(); }
    void startNetwork()    override { ledStrip.startUDP(); }
    void update()          override { ledStrip.update(); }
    uint32_t updateIntervalMs() override { return 0; }  // every loop

    void getLiveData(JsonObject& out) override {
        out["on"]         = ledStrip.isOn();
        out["r"]          = ledStrip.r();
        out["g"]          = ledStrip.g();
        out["b"]          = ledStrip.b();
        out["brightness"] = ledStrip.brightness();
        out["ddp_active"] = ledStrip.isDDPActive();
    }
};
