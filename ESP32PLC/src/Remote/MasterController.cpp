#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>
#include "Devices/Log.h"
#include "MQTT.h"
#include "FileSystem/FSInterface.h"

ModbusMasterController master(Serial1, 38400, 16, 17, 20);
static RemoteMaster    remoteMaster;

// MQTT publish bridge — called by RemoteMaster for online/offline events.
// Prepends "ESPPLC/<hostname>/" so device status lands under the same root as all other topics.
static void _mqttPublish(const char* subTopic, const char* payload) {
    if (!GetMQTTStatus()) return;
    char fullTopic[128];
    snprintf(fullTopic, sizeof(fullTopic), "%s/%s", GetMQTTBaseTopic(), subTopic);
    GetMQTTClient().publish(fullTopic, payload);
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

// ----------------------------------------------------------------
// Modbus bus scan — probes each address with a FC3 read (1 register)
// Runs as a FreeRTOS task so it doesn't block the web server.
// ----------------------------------------------------------------
static constexpr uint8_t MB_TX_EN = 20;  // matches master constructor pin

static uint16_t _mbCRC(const uint8_t* b, uint8_t n) {
    uint16_t c = 0xFFFF;
    for (uint8_t i = 0; i < n; i++) {
        c ^= b[i];
        for (uint8_t j = 0; j < 8; j++)
            c = (c & 1u) ? (c >> 1) ^ 0xA001u : (c >> 1);
    }
    return c;
}

static bool _probeAddr(uint8_t addr) {
    uint8_t req[8] = { addr, 0x03, 0x00, 0x00, 0x00, 0x01, 0, 0 };
    uint16_t crc = _mbCRC(req, 6);
    req[6] = (uint8_t)(crc & 0xFF);
    req[7] = (uint8_t)(crc >> 8);

    // 1. Ensure receive mode and clear stale RX bytes
    digitalWrite(MB_TX_EN, LOW);
    delay(2);
    while (Serial1.available()) Serial1.read();

    // 2. Enable driver, brief DE-setup, then send frame
    digitalWrite(MB_TX_EN, HIGH);
    delayMicroseconds(200);
    Serial1.write(req, 8);
    // Time-based TX wait instead of flush():
    //   8 bytes × 10 bits / 38400 baud = 2.08 ms → 6 ms gives 3× margin and
    //   avoids relying on flush() semantics (varies between ESP32 core versions).
    delay(6);
    digitalWrite(MB_TX_EN, LOW);
    delayMicroseconds(500);  // RE enable + Modbus turnaround guard

    // 3. Discard any echo bytes (transceivers that keep RX live during TX)
    while (Serial1.available()) Serial1.read();

    // 4. Wait for slave response (FC3 normal = 7 B, error = 5 B)
    unsigned long t = millis();
    while (Serial1.available() < 4) {
        if (millis() - t > 200) return false;
        delay(2);
    }
    uint8_t first = (uint8_t)Serial1.read();
    delay(15);
    while (Serial1.available()) Serial1.read();  // drain rest of frame
    return (first == addr);
}

static volatile bool    _scanRunning = false;
static volatile uint8_t _scanProg    = 0;
static volatile uint8_t _scanTotal   = 0;
static char             _scanResult[1024] = "[]"; // up to 247 addresses × 4 chars each

static struct { uint8_t from; uint8_t to; } _scanArg;

static void _scanTask(void*) {
    delay(300);  // let any in-flight Modbus transaction drain
    uint8_t from = _scanArg.from;
    uint8_t to   = _scanArg.to;
    _scanTotal   = (uint8_t)(to - from + 1);
    _scanProg    = 0;

    // Build into a private static buffer so _scanResult (exposed via HTTP)
    // stays as valid "[]" until the scan is 100% done.  Avoids the browser
    // seeing unclosed JSON like "[5,10" mid-scan and stopping polling early.
    static char _build[1024];
    _build[0] = '[';
    char* p   = _build + 1;
    bool first = true;

    for (uint8_t a = from; a <= to; a++) {
        _scanProg++;
        if (_probeAddr(a)) {
            Log(NOTIFY_FORCE, "Scan: addr %u responded\r\n", (unsigned)a);
            size_t rem = sizeof(_build) - (size_t)(p - _build) - 4;
            if (!first && rem > 4) *p++ = ',';
            p += snprintf(p, rem, "%u", (unsigned)a);
            first = false;
        }
        delay(5);
    }
    *p++ = ']';
    *p   = '\0';

    // Now atomically publish the complete result
    strlcpy(_scanResult, _build, sizeof(_scanResult));
    Log(NOTIFY_FORCE, "Scan complete: %s\r\n", _scanResult);
    ResumeRemotePolling();
    _scanRunning = false;
    vTaskDelete(nullptr);
}

void StartBusScan(uint8_t from, uint8_t to) {
    if (_scanRunning) return;
    _scanArg     = { from, to };
    _scanRunning = true;
    _scanProg    = 0;
    _scanTotal   = (uint8_t)(to - from + 1);
    strlcpy(_scanResult, "[]", sizeof(_scanResult));
    SuspendRemotePolling();
    xTaskCreate(_scanTask, "mbScan", 3072, nullptr, 1, nullptr);
}

bool        IsBusScanRunning()  { return _scanRunning; }
const char* GetBusScanResult()  { return _scanResult;  }
uint8_t     GetBusScanProgress(){ return _scanProg;    }
uint8_t     GetBusScanTotal()   { return _scanTotal;   }
