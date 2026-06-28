#include "FireworksModule.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <string.h>
#include "Remote/MasterController.h"
#include "FileSystem/FSInterface.h"
#include "Devices/Log.h"

FireworksModule fwModule;

// ── begin: scan Remote.json for devices with typeId == HPO_DEVICE_TYPE ────────
void FireworksModule::begin() {
    _devCount = 0;
    const RemoteConfig_t& cfg = GetRemoteConfig();
    for (uint8_t i = 0; i < cfg.deviceCount && _devCount < FW_MOD_MAX_DEVICES; i++) {
        if (cfg.devices[i].typeId == HPO_DEVICE_TYPE) {
            _devs[_devCount].addr    = cfg.devices[i].address;
            _devs[_devCount].pollDev = nullptr;
            strlcpy(_devs[_devCount].name, cfg.devices[i].name, sizeof(_devs[0].name));
            // Find the "Power" polling group (FC4, startReg=2) for SafetyV/VIN reads
            for (uint8_t g = 0; g < RemoteGrpCount(); g++) {
                ModbusDevice* gd = RemoteGrpDevice(g);
                if (gd && gd->getAddress() == cfg.devices[i].address
                       && gd->getRemoteReg() == HPO_POWER_START_REG) {
                    _devs[_devCount].pollDev = gd;
                    break;
                }
            }
            Log(LOG, "FW: found HPO device [%s] addr=0x%02X pollDev=%s\r\n",
                cfg.devices[i].name, cfg.devices[i].address,
                _devs[_devCount].pollDev ? "linked" : "NOT FOUND");
            _devCount++;
        }
    }
    if (_devCount == 0)
        Log(NOTIFY, "FW: no HighPowerOutput devices found in config (typeId=%u)\r\n", HPO_DEVICE_TYPE);
}

// ── update: step through sequence ─────────────────────────────────────────────
void FireworksModule::update() {
    if (!_seqRunning || _seqLen == 0) return;

    // Process all steps that are immediately ready (batches delay=0 steps into
    // a single update call so they queue back-to-back with no 10ms gap between them)
    while (_seqStep < _seqLen) {
        const Step& s = _seq[_seqStep];
        if ((millis() - _seqLastMs) < s.delayMs) break;

        if (s.devIdx < _devCount)
            _fireOutput(_devs[s.devIdx].addr, s.output);

        _seqLastMs = millis();
        _seqStep++;
    }

    if (_seqStep >= _seqLen) {
        _seqRunning = false;
        Log(LOG, "FW: sequence complete (%u steps)\r\n", (unsigned)_seqLen);
    }
}

// ── getLiveData ────────────────────────────────────────────────────────────────
void FireworksModule::getLiveData(JsonObject& out) {
    out["device_count"] = _devCount;
    out["seq_running"]  = _seqRunning;
    out["seq_step"]     = _seqStep;
    out["seq_len"]      = _seqLen;

    bool allSafe = (_devCount > 0);
    JsonArray devArr = out.createNestedArray("devices");
    for (uint8_t i = 0; i < _devCount; i++) {
        JsonObject d = devArr.createNestedObject();
        d["idx"]  = i;
        d["addr"] = _devs[i].addr;
        d["name"] = _devs[i].name;
        float v  = _getSafetyV(i);
        bool  ok = (v >= HPO_SAFETY_V_MIN);
        d["safety_v"]  = v;
        d["safety_ok"] = ok;
        if (!ok) allSafe = false;
    }
    out["safety_ok"] = allSafe;
}

// ── safety voltage helpers ─────────────────────────────────────────────────────
float FireworksModule::_getSafetyV(uint8_t devIdx) const {
    if (devIdx >= _devCount || !_devs[devIdx].pollDev) return 0.0f;
    return _devs[devIdx].pollDev->getFloat(HPO_REG_SAFETY_V);
}

bool FireworksModule::_isSafetyOk(uint8_t devIdx) const {
    return _getSafetyV(devIdx) >= HPO_SAFETY_V_MIN;
}

// ── registerRoutes ─────────────────────────────────────────────────────────────
void FireworksModule::registerRoutes(AsyncWebServer& svr) {
    // GET /api/fireworks/status
    svr.on("/api/fireworks/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        AsyncResponseStream* resp = req->beginResponseStream("application/json");
        StaticJsonDocument<768> doc;
        JsonObject obj = doc.to<JsonObject>();
        this->getLiveData(obj);
        serializeJson(doc, *resp);
        req->send(resp);
    });

    // POST /api/fireworks/mode  body: {"enter":true}
    svr.on("/api/fireworks/mode", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<64> doc;
            bool enter = false;
            if (deserializeJson(doc, data, len) == DeserializationError::Ok)
                enter = doc["enter"] | false;

            uint16_t cmd = enter ? HPO_MODE_CMD_ENTER_FW : HPO_MODE_CMD_EXIT_FW;
            for (uint8_t i = 0; i < _devCount; i++)
                _sendModeCmd(_devs[i].addr, cmd);

            Log(LOG, "FW: mode command %s sent to %u device(s)\r\n",
                enter ? "ENTER_FW" : "EXIT_FW", (unsigned)_devCount);
            req->send(200, "application/json", "{\"ok\":true}");
        });

    // POST /api/fireworks/sequence
    // body: [{"dev":0,"out":1,"delay_ms":0}, ...]
    svr.on("/api/fireworks/sequence", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            _seqRunning = false;
            _seqLen     = 0;
            _seqStep    = 0;

            StaticJsonDocument<1024> doc;
            if (deserializeJson(doc, data, len) != DeserializationError::Ok) {
                req->send(400, "application/json", "{\"error\":\"bad JSON\"}");
                return;
            }
            JsonArray arr = doc.as<JsonArray>();
            if (!arr) {
                req->send(400, "application/json", "{\"error\":\"expected array\"}");
                return;
            }
            for (JsonVariant v : arr) {
                if (_seqLen >= FW_MOD_MAX_STEPS) break;
                Step& s   = _seq[_seqLen];
                s.devIdx  = v["dev"]      | 0;
                s.output  = v["out"]      | 1;
                s.delayMs = v["delay_ms"] | 0;
                if (s.output < 1 || s.output > HPO_OUTPUTS_PER_DEV) s.output = 1;
                if (s.devIdx >= _devCount)                           s.devIdx = 0;
                _seqLen++;
            }
            char resp[48];
            snprintf(resp, sizeof(resp), "{\"ok\":true,\"steps\":%u}", (unsigned)_seqLen);
            req->send(200, "application/json", resp);
            Log(LOG, "FW: sequence stored (%u steps)\r\n", (unsigned)_seqLen);
        });

    // POST /api/fireworks/fire
    svr.on("/api/fireworks/fire", HTTP_POST, [this](AsyncWebServerRequest* req) {
        if (_seqLen == 0) {
            req->send(400, "application/json", "{\"ok\":false,\"error\":\"no sequence loaded\"}");
            return;
        }
        // Block fire if any device has insufficient safety voltage
        for (uint8_t i = 0; i < _devCount; i++) {
            if (!_isSafetyOk(i)) {
                char err[96];
                snprintf(err, sizeof(err),
                    "{\"ok\":false,\"error\":\"Safety voltage low on Dev%u (%.1fV < %.0fV)\"}",
                    (unsigned)i, (double)_getSafetyV(i), (double)HPO_SAFETY_V_MIN);
                Log(ERROR, "FW: FIRE BLOCKED — %s\r\n", err);
                req->send(403, "application/json", err);
                return;
            }
        }
        _seqStep    = 0;
        _seqLastMs  = millis();
        _seqRunning = true;
        Log(LOG, "FW: sequence started (%u steps)\r\n", (unsigned)_seqLen);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // POST /api/fireworks/abort
    svr.on("/api/fireworks/abort", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _abortSequence();
        req->send(200, "application/json", "{\"ok\":true}");
    });
}

// ── helpers ────────────────────────────────────────────────────────────────────
void FireworksModule::_fireOutput(uint8_t addr, uint8_t output) {
    uint16_t coil = HPO_COIL_FIRE_BASE + (output - 1);
    master.queueWriteCoil(addr, coil, true);
    Log(LOG, "FW: fire addr=0x%02X out=%u coil=%u\r\n",
        (unsigned)addr, (unsigned)output, (unsigned)coil);
}

void FireworksModule::_sendModeCmd(uint8_t addr, uint16_t cmd) {
    master.queueWrite(addr, 0, cmd);
}

void FireworksModule::_abortSequence() {
    _seqRunning = false;
    for (uint8_t di = 0; di < _devCount; di++) {
        for (uint8_t out = 0; out < HPO_OUTPUTS_PER_DEV; out++)
            master.queueWriteCoil(_devs[di].addr, HPO_COIL_FIRE_BASE + out, false);
    }
    Log(LOG, "FW: sequence aborted — all outputs cleared\r\n");
}
