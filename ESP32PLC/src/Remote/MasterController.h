#ifndef MASTERCONTROLLER_H
#define MASTERCONTROLLER_H

#include <ESP32ModbusMaster.h>
#include "Remote/RemoteConfig.h"

// Legacy register index defines — kept for MQTT.cpp stubs (all return 0)
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

extern ModbusMasterController master;

// Group pool — one slot per active (device × group) pair
uint8_t        RemoteGrpCount();
ModbusDevice*  RemoteGrpDevice(uint8_t grpPoolIdx);
uint8_t        RemoteGrpDevIdx(uint8_t grpPoolIdx);
uint8_t        RemoteGrpGrpIdx(uint8_t grpPoolIdx);
ModuleStatus_t RemoteDevStatus(uint8_t devIdx);

// Setup / runtime
char RemoteStart();
void RemoteRun();

// Occupancy sensor accessors
bool     GetOccupied();
uint16_t GetZoneCount();
uint16_t GetTimeOnSec();

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

/* ── Modbus bus address scan ────────────────────────────────────────────── */
void        StartBusScan(uint8_t fromAddr = 1, uint8_t toAddr = 247);
bool        IsBusScanRunning();
const char* GetBusScanResult();   // JSON array string, e.g. "[5,22]"
uint8_t     GetBusScanProgress(); // addresses probed so far
uint8_t     GetBusScanTotal();    // total addresses in range

/* ── Device list for FW update UI ──────────────────────────────────────── */
using FwDeviceInfo = RemoteMaster::DeviceInfo;

/* Fills 'out' with up to maxCount entries. Returns actual count. */
uint8_t GetFwDeviceList(FwDeviceInfo *out, uint8_t maxCount);

#endif
