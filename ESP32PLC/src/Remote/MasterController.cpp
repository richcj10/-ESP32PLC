#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>
#include "Devices/Log.h"
#include "MQTT.h"
#include "FileSystem/FSInterface.h"

ModbusMasterController master(Serial1, 38400, 16, 17, 20);
static RemoteMaster    remoteMaster;

// MQTT publish bridge — called by RemoteMaster for online/offline events
static void _mqttPublish(const char* topic, const char* payload) {
    if (GetMQTTStatus()) GetMQTTClient().publish(topic, payload);
}

// ----------------------------------------------------------------
// Public group pool accessors
// ----------------------------------------------------------------
uint8_t        RemoteGrpCount()                { return remoteMaster.grpCount(); }
ModbusDevice*  RemoteGrpDevice(uint8_t g)      { return remoteMaster.grpDevice(g); }
uint8_t        RemoteGrpDevIdx(uint8_t g)      { return remoteMaster.grpDevIdx(g); }
uint8_t        RemoteGrpGrpIdx(uint8_t g)      { return remoteMaster.grpGrpIdx(g); }
ModuleStatus_t RemoteDevStatus(uint8_t devIdx) { return remoteMaster.devStatus(devIdx); }

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
char RemoteStart() {
    remoteMaster.setPublishCallback(_mqttPublish);

    if (!remoteMaster.begin(master, GetRemoteConfig())) {
        // No JSON config — fall back to hardcoded OCC at address 22
        Log(NOTIFY, "Remote: no JSON config — using hardcoded OCC defaults\r\n");
        ModbusDevice* dev = new ModbusDevice(22, READ_INPUT_REGISTERS, 0, 3);
        dev->setPollRate(500);
        master.addDevice(*dev);
        master.begin();
    }
    return 1;
}

// ----------------------------------------------------------------
// RS-485 bus suspend / resume for FW update
// ----------------------------------------------------------------
void SuspendRemotePolling() { remoteMaster.suspend(); }
void ResumeRemotePolling()  { remoteMaster.resume(); }
bool IsRemotePollingActive(){ return !remoteMaster.suspended(); }

// ----------------------------------------------------------------
// Device list for FW update UI
// ----------------------------------------------------------------
uint8_t GetFwDeviceList(FwDeviceInfo *out, uint8_t maxCount) {
    return remoteMaster.getDeviceList(out, maxCount);
}

// ----------------------------------------------------------------
// Runtime
// ----------------------------------------------------------------
void RemoteRun() {
    remoteMaster.update();
}

// ----------------------------------------------------------------
// GetModuleValue — for PLCModuleDesc_t typed devices
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
    const char* s = inst->status == MODULE_VALID   ? "VALID"   :
                    inst->status == MODULE_INVALID ? "INVALID" :
                    inst->status == MODULE_OFFLINE ? "OFFLINE" : "UNKNOWN";
    Log(LOG, "[%s addr=%u] %s\r\n", desc->name, inst->address, s);
}

// ----------------------------------------------------------------
// Occupancy accessors — first group of first device
// ----------------------------------------------------------------
bool     GetOccupied()  { return remoteMaster.grpCount() > 0 ? remoteMaster.grpDevice(0)->getRaw(0) != 0 : false; }
uint16_t GetZoneCount() { return remoteMaster.grpCount() > 0 ? remoteMaster.grpDevice(0)->getRaw(1) : 0; }
uint16_t GetTimeOnSec() { return remoteMaster.grpCount() > 0 ? remoteMaster.grpDevice(0)->getRaw(2) : 0; }

// ----------------------------------------------------------------
// Stubs — legacy offline devices
// ----------------------------------------------------------------
char  ReadRemoteTemp()    { return 0; }
char  ReadRemoteWeather() { return 0; }
char  ReadRemoteCurrent() { return 0; }

float GetRemoteDataFromQue(unsigned char x, bool Divide) { (void)x; (void)Divide; return 0.0f; }

void SetTempPollRate   (unsigned long ms) { (void)ms; }
void SetWeatherPollRate(unsigned long ms) { (void)ms; }
void SetCurrentPollRate(unsigned long ms) { (void)ms; }

char SetOcupyLED(int Address, unsigned char R, unsigned char G, unsigned char B) {
    uint16_t vals[3] = { 12, R, (uint16_t)((G << 8) | B) };
    return master.queueWriteMulti((uint8_t)Address, 0x02, vals, 3) ? 1 : 0;
}

int   ReadDeviceType(int Address) { (void)Address; return 0; }
float readDeviceVIN (int Address) { (void)Address; return -1; }
char  GetRemoteTemp (char TempCH) { (void)TempCH;  return 0; }
