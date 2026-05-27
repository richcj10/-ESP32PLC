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

    const RemoteConfig_t& cfg = GetRemoteConfig();
    Log(NOTIFY, "Remote: config loaded=%s deviceCount=%u\r\n",
        cfg.loaded ? "yes" : "NO", (unsigned)cfg.deviceCount);

    if (cfg.loaded) {
        for (uint8_t i = 0; i < cfg.deviceCount; i++) {
            const RemoteDeviceCfg_t& d = cfg.devices[i];
            Log(NOTIFY, "Remote:   dev[%u] name=%s addr=%u groups=%u typeId=%u swVer=0x%04X\r\n",
                i, d.name, (unsigned)d.address, (unsigned)d.groupCount,
                (unsigned)d.typeId, (unsigned)d.swVersion);
            for (uint8_t g = 0; g < d.groupCount; g++) {
                const RemoteGroupCfg_t& gr = d.groups[g];
                Log(NOTIFY, "Remote:     grp[%u] name=%s fc=%u reg=%u+%u poll=%lums\r\n",
                    g, gr.name, (unsigned)gr.fc, (unsigned)gr.startReg,
                    (unsigned)gr.count, (unsigned long)gr.pollMs);
            }
        }
    }

    if (!remoteMaster.begin(master, cfg)) {
        // No JSON config — fall back to hardcoded OCC at address 22
        Log(NOTIFY, "Remote: no JSON config — using hardcoded OCC defaults\r\n");
        ModbusDevice* dev = new ModbusDevice(22, READ_INPUT_REGISTERS, 0, 3);
        dev->setPollRate(500);
        master.addDevice(*dev);
        master.begin();
    }

    Log(NOTIFY, "Remote: polling started — %u group(s) registered\r\n",
        (unsigned)remoteMaster.grpCount());
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

bool QueueModbusWriteRegs(uint8_t addr, uint16_t startReg,
                          const uint16_t *values, uint8_t count) {
    return master.queueWriteMulti(addr, startReg, values, count);
}

// ----------------------------------------------------------------
// Runtime
// ----------------------------------------------------------------
void RemoteRun() {
    remoteMaster.update();
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

// Read FC4 reg 0 (version) and reg 1 (typeId) — called only for addresses that
// already responded to _probeAddr(), so the bus is quiet and timing is clean.
static bool _probeFC4(uint8_t addr, uint16_t* ver, uint16_t* tid) {
    uint8_t req[8] = { addr, 0x04, 0x00, 0x00, 0x00, 0x02, 0, 0 };
    uint16_t crc = _mbCRC(req, 6);
    req[6] = (uint8_t)(crc & 0xFF);
    req[7] = (uint8_t)(crc >> 8);

    digitalWrite(MB_TX_EN, LOW);
    delay(2);
    while (Serial1.available()) Serial1.read();

    digitalWrite(MB_TX_EN, HIGH);
    delayMicroseconds(200);
    Serial1.write(req, 8);
    delay(6);
    digitalWrite(MB_TX_EN, LOW);
    delayMicroseconds(500);
    while (Serial1.available()) Serial1.read();

    unsigned long t = millis();
    while (Serial1.available() < 9) {
        if (millis() - t > 200) return false;
        delay(2);
    }
    delay(5);
    uint8_t resp[16]; uint8_t n = 0;
    while (Serial1.available() && n < 16) resp[n++] = (uint8_t)Serial1.read();

    if (n < 9 || resp[0] != addr || resp[1] != 0x04 || resp[2] != 4) return false;
    uint16_t gotCrc = (uint16_t)resp[n-2] | ((uint16_t)resp[n-1] << 8);
    if (gotCrc != _mbCRC(resp, n-2)) return false;

    *ver = ((uint16_t)resp[3] << 8) | resp[4];
    *tid = ((uint16_t)resp[5] << 8) | resp[6];
    return true;
}

static volatile bool    _scanRunning = false;
static volatile uint8_t _scanProg    = 0;
static volatile uint8_t _scanTotal   = 0;
static char             _scanResult[1024] = "[]"; // up to 247 addresses × 4 chars each
static char             _probeResult[512] = "[]"; // {a,v,t} per responding device

static struct { uint8_t from; uint8_t to; } _scanArg;

static void _scanTask(void*) {
    delay(300);  // let any in-flight Modbus transaction drain
    uint8_t from = _scanArg.from;
    uint8_t to   = _scanArg.to;
    _scanTotal   = (uint8_t)(to - from + 1);
    _scanProg    = 0;

    // Build into private static buffers so the externally-visible strings
    // stay as "[]" until the scan is fully done (avoids partial JSON in browser).
    static char _build[1024];
    _build[0] = '[';
    char* p    = _build + 1;
    bool  first = true;

    static char _pbuild[512];
    _pbuild[0] = '[';
    char* pp    = _pbuild + 1;
    bool  pfirst = true;

    for (uint8_t a = from; a <= to; a++) {
        _scanProg++;
        if (_probeAddr(a)) {
            Log(NOTIFY_FORCE, "Scan: addr %u responded\r\n", (unsigned)a);
            size_t rem = sizeof(_build) - (size_t)(p - _build) - 4;
            if (!first && rem > 4) *p++ = ',';
            p += snprintf(p, rem, "%u", (unsigned)a);
            first = false;

            uint16_t ver = 0, tid = 0;
            if (_probeFC4(a, &ver, &tid)) {
                size_t prem = sizeof(_pbuild) - (size_t)(pp - _pbuild) - 4;
                if (prem > 32) {
                    if (!pfirst) *pp++ = ',';
                    pp += snprintf(pp, prem, "{\"a\":%u,\"v\":%u,\"t\":%u}",
                                   (unsigned)a, (unsigned)ver, (unsigned)tid);
                    pfirst = false;
                }
            }
        }
        delay(5);
    }
    *p++ = ']'; *p  = '\0';
    *pp++ = ']'; *pp = '\0';

    // Atomically publish complete results
    strlcpy(_scanResult,  _build,  sizeof(_scanResult));
    strlcpy(_probeResult, _pbuild, sizeof(_probeResult));
    Log(NOTIFY_FORCE, "Scan complete: %s  probe: %s\r\n", _scanResult, _probeResult);
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
    strlcpy(_scanResult,  "[]", sizeof(_scanResult));
    strlcpy(_probeResult, "[]", sizeof(_probeResult));
    SuspendRemotePolling();
    xTaskCreate(_scanTask, "mbScan", 3072, nullptr, 1, nullptr);
}

bool        IsBusScanRunning()  { return _scanRunning;  }
const char* GetBusScanResult()  { return _scanResult;   }
const char* GetBusScanProbe()   { return _probeResult;  }
uint8_t     GetBusScanProgress(){ return _scanProg;     }
uint8_t     GetBusScanTotal()   { return _scanTotal;    }
