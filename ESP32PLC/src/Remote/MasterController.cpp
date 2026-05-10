#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>
#include "Devices/Log.h"
#include "MQTT.h"

#include "PLCMod_OCC.h"

// ----------------------------------------------------------------
// Active device
// ----------------------------------------------------------------
ModbusDevice occSensor(22, READ_INPUT_REGISTERS, 0, 3);

ModbusMasterController master(Serial1, 38400, 16, 17, 20);

// ----------------------------------------------------------------
// Module instance
// ----------------------------------------------------------------
PLCModuleInstance_t occInst = { &OCC_Desc, &occSensor, 22, MODULE_UNKNOWN };

// ----------------------------------------------------------------
// Stubbed-out devices — kept so callers (MQTT, Sensors) still compile.
// Not added to master; will never respond.
// ----------------------------------------------------------------
ModbusDevice tempSensor   (32, READ_INPUT_REGISTERS, 3, 3);
ModbusDevice weatherSensor(48, READ_INPUT_REGISTERS, 0, 7);
ModbusDevice currentSensor( 5, READ_INPUT_REGISTERS, 1, 5);

PLCModuleInstance_t tempInst    = { nullptr, &tempSensor,    32, MODULE_OFFLINE };
PLCModuleInstance_t weatherInst = { nullptr, &weatherSensor, 48, MODULE_OFFLINE };
PLCModuleInstance_t currentInst = { nullptr, &currentSensor,  5, MODULE_OFFLINE };

// ----------------------------------------------------------------
// Comms status MQTT helper
// ----------------------------------------------------------------
static void publishDeviceStatus(const char* name, ModbusDevice* d, bool online) {
    char buf[48];
    if (online)
        snprintf(buf, sizeof(buf), "online");
    else
        snprintf(buf, sizeof(buf), "offline | errors: %lu", d->failedRequests());
    char topic[40];
    snprintf(topic, sizeof(topic), "Devices/%s/comms", name);
    Log(online ? LOG : NOTIFY, "Device %s %s\r\n", name, buf);
    if (GetMQTTStatus()) GetMQTTClient().publish(topic, buf);
}

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
char RemoteStart() {
    occSensor.setPollRate(500);

    occSensor.onStatusChange([](ModbusDevice* d, bool online) {
        if (online) CheckModuleVersion(&occInst);
        else        occInst.status = MODULE_OFFLINE;
        publishDeviceStatus("OccSensor", d, online);
    });

    occSensor.onDataReceived([](ModbusDevice* d) {
        if (occInst.status != MODULE_VALID) return;
        Log(DEBUG, "OCC ok | occupied=%u zones=%u time=%u\r\n",
            d->getRaw(0), d->getRaw(1), d->getRaw(2));
    });

    master.addDevice(occSensor);
    master.begin();
    return 1;
}

// ----------------------------------------------------------------
// RS-485 bus suspend / resume for FW update
// ----------------------------------------------------------------
static volatile bool _pollingSuspended = false;

void SuspendRemotePolling() { _pollingSuspended = true; }
void ResumeRemotePolling()  { _pollingSuspended = false; }
bool IsRemotePollingActive(){ return !_pollingSuspended; }

// ----------------------------------------------------------------
// Device list for FW update UI
// ----------------------------------------------------------------
static const FwDeviceInfo _fwDevices[] = {
    { "OCC-TOL Sensor", 22 },
};

uint8_t GetFwDeviceList(FwDeviceInfo *out, uint8_t maxCount) {
    uint8_t count = sizeof(_fwDevices) / sizeof(_fwDevices[0]);
    if (count > maxCount) count = maxCount;
    for (uint8_t i = 0; i < count; i++) out[i] = _fwDevices[i];
    return count;
}

// ----------------------------------------------------------------
// Runtime
// ----------------------------------------------------------------
void RemoteRun() {
    if (_pollingSuspended) return;
    master.update();

    static unsigned long lastStatDump = 0;
    if (millis() - lastStatDump > 10000) {
        lastStatDump = millis();
        Log(DEBUG, "OCC req:%lu ok:%lu fail:%lu\r\n",
            occSensor.totalRequests(), occSensor.successRequests(), occSensor.failedRequests());
    }
}

// ----------------------------------------------------------------
// GetModuleValue — scale raw register using group descriptor
// ----------------------------------------------------------------
float GetModuleValue(const PLCModuleInstance_t* inst, uint8_t localIdx) {
    if (!inst || inst->status != MODULE_VALID || !inst->desc || !inst->device) return 0.0f;

    const PLCModuleDesc_t* desc = inst->desc;
    for (uint8_t g = 0; g < desc->groupCount; g++) {
        const RegGroup_t* grp = &desc->groups[g];
        uint8_t end = grp->startReg + grp->count;
        if (localIdx < grp->startReg || localIdx >= end) continue;

        if (grp->type == RTYPE_COIL || grp->type == RTYPE_DISCRETE)
            return inst->device->getCoil(localIdx) ? 1.0f : 0.0f;
        if (grp->scale == 1.0f) return (float)inst->device->getRaw(localIdx);
        return (float)inst->device->getSigned(localIdx) / grp->scale;
    }
    return 0.0f;
}

// ----------------------------------------------------------------
// LogModuleData
// ----------------------------------------------------------------
void LogModuleData(const PLCModuleInstance_t* inst) {
    if (!inst || !inst->desc) return;

    const PLCModuleDesc_t* desc = inst->desc;
    const char* statusStr =
        inst->status == MODULE_VALID   ? "VALID"   :
        inst->status == MODULE_INVALID ? "INVALID" :
        inst->status == MODULE_OFFLINE ? "OFFLINE" : "UNKNOWN";

    Log(LOG, "[%s addr=%u] %s\r\n", desc->name, inst->address, statusStr);
    if (inst->status != MODULE_VALID) return;

    for (uint8_t g = 0; g < desc->groupCount; g++) {
        const RegGroup_t* grp = &desc->groups[g];
        if (grp->scanMode == SCAN_ON_DEMAND) continue;
        for (uint8_t r = 0; r < grp->count; r++) {
            float val = GetModuleValue(inst, grp->startReg + r);
            Log(LOG, "  %-14s = %.2f %s\r\n", RegGroupGetName(grp, r), val, grp->units);
        }
    }
}

// ----------------------------------------------------------------
// Occupancy accessors
// ----------------------------------------------------------------
bool GetOccupied()     { return occSensor.getRaw(0) != 0; }
uint16_t GetZoneCount(){ return occSensor.getRaw(1); }
uint16_t GetTimeOnSec(){ return occSensor.getRaw(2); }

// ----------------------------------------------------------------
// Stubbed diagnostic helpers — old devices are offline
// ----------------------------------------------------------------
char ReadRemoteTemp()    { LogModuleData(&tempInst);    return 0; }
char ReadRemoteWeather() { LogModuleData(&weatherInst); return 0; }
char ReadRemoteCurrent() { LogModuleData(&currentInst); return 0; }

// ----------------------------------------------------------------
// GetRemoteDataFromQue — legacy API (old devices offline, returns 0)
// ----------------------------------------------------------------
float GetRemoteDataFromQue(unsigned char x, bool Divide) {
    (void)x; (void)Divide;
    return 0.0f;
}

// ----------------------------------------------------------------
// Poll rate stubs — old devices not active
// ----------------------------------------------------------------
void SetTempPollRate   (unsigned long ms) { (void)ms; }
void SetWeatherPollRate(unsigned long ms) { (void)ms; }
void SetCurrentPollRate(unsigned long ms) { (void)ms; }

// ----------------------------------------------------------------
// Write helpers
// ----------------------------------------------------------------
char SetOcupyLED(int Address, unsigned char R, unsigned char G, unsigned char B) {
    uint16_t vals[3] = { 12, R, (uint16_t)((G << 8) | B) };
    return master.queueWriteMulti((uint8_t)Address, 0x02, vals, 3) ? 1 : 0;
}

// ----------------------------------------------------------------
// Stubs
// ----------------------------------------------------------------
int   ReadDeviceType(int Address) { (void)Address; return 0; }
float readDeviceVIN (int Address) { (void)Address; return -1; }
char  GetRemoteTemp (char TempCH) { (void)TempCH;  return 0; }
