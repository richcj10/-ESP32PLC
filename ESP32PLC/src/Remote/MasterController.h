#ifndef MASTERCONTROLLER_H
#define MASTERCONTROLLER_H

#include "ModbusDevice.h"
#include "ModbusMasterController.h"
#include "Remote/RemoteConfig.h"

// Legacy absolute register index defines — kept for MQTT.cpp / other callers
#define REMOTE_TEMP_RTD_1  1
#define REMOTE_TEMP_RTD_2  2
#define REMOTE_TEMP_RTD_3  3

#define OUTSIDE_TEMP_POS   5
#define OUTSIDE_HUMID_POS  6
#define MC_POS             7
#define WIND_SPEED_POS     8
#define WIND_DIR_POS       9
#define RAIN_POS           10

#define REMOTE_CS_A        13
#define REMOTE_CS_B        14
#define REMOTE_CS_C        15

// Active device
extern ModbusDevice           occSensor;
extern ModbusMasterController master;
extern PLCModuleInstance_t    occInst;

// Stubbed-out devices — not polled, kept for legacy callers
extern ModbusDevice           tempSensor;
extern ModbusDevice           weatherSensor;
extern ModbusDevice           currentSensor;
extern PLCModuleInstance_t    tempInst;
extern PLCModuleInstance_t    weatherInst;
extern PLCModuleInstance_t    currentInst;

// Setup / runtime
char RemoteStart();
void RemoteRun();

// Occupancy sensor accessors
bool     GetOccupied();
uint16_t GetZoneCount();
uint16_t GetTimeOnSec();

// Diagnostic log — prints all SCAN_ALWAYS groups for a module
void LogModuleData(const PLCModuleInstance_t* inst);

// Read a register value using the descriptor's scale factor.
// Returns 0.0 if the instance is not MODULE_VALID.
float GetModuleValue(const PLCModuleInstance_t* inst, uint8_t localIdx);

// Legacy C-style API — unchanged so existing callers compile without modification
char  ReadRemoteTemp();
char  ReadRemoteWeather();
char  ReadRemoteCurrent();
float GetRemoteDataFromQue(unsigned char x, bool Divide);
int   ReadDeviceType(int Address);
float readDeviceVIN(int Address);
char  SetOcupyLED(int Address, unsigned char R, unsigned char G, unsigned char B);
char  GetRemoteTemp(char TempCH);

void SetTempPollRate   (unsigned long ms);
void SetWeatherPollRate(unsigned long ms);
void SetCurrentPollRate(unsigned long ms);

/* ── RS-485 bus control for FW update ─────────────────────────────────────
 * Call SuspendRemotePolling() before handing Serial1 to ModBusBLMaster.
 * Call ResumeRemotePolling()  when the flash task is complete.
 * ──────────────────────────────────────────────────────────────────────── */
void SuspendRemotePolling();
void ResumeRemotePolling();
bool IsRemotePollingActive();

/* ── Device list for FW update UI ──────────────────────────────────────── */
struct FwDeviceInfo {
    const char *name;
    uint8_t     slaveId;
};

/* Fills 'out' with up to maxCount entries. Returns actual count. */
uint8_t GetFwDeviceList(FwDeviceInfo *out, uint8_t maxCount);

#endif
