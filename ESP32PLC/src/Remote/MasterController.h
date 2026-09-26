#ifndef MASTERCONTROLLER_H
#define MASTERCONTROLLER_H

#include <ESP32ModbusMaster.h>
#include "Remote/RemoteConfig.h"

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

// Occupancy sensor accessor
bool     GetOccupied();

/* ── RS-485 bus control for FW update ─────────────────────────────────────
 * Call SuspendRemotePolling() before handing Serial1 to ModBusBLMaster.
 * Call ResumeRemotePolling()  when the flash task is complete.
 * ──────────────────────────────────────────────────────────────────────── */
void SuspendRemotePolling();
void ResumeRemotePolling();

/* ── Modbus bus address scan ────────────────────────────────────────────── */
void        StartBusScan(uint8_t fromAddr = 1, uint8_t toAddr = 247);
bool        IsBusScanRunning();
const char* GetBusScanResult();   // JSON array string, e.g. "[5,22]"
const char* GetBusScanProbe();    // JSON array of {a,v,t} per responding device
uint8_t     GetBusScanProgress(); // addresses probed so far
uint8_t     GetBusScanTotal();    // total addresses in range

/* ── Device list for FW update UI ──────────────────────────────────────── */
using FwDeviceInfo = RemoteMaster::DeviceInfo;

/* Fills 'out' with up to maxCount entries. Returns actual count. */
uint8_t GetFwDeviceList(FwDeviceInfo *out, uint8_t maxCount);

/* ── Ad-hoc Modbus write (FC16 multiple holding registers) ─────────────── */
bool QueueModbusWriteRegs(uint8_t addr, uint16_t startReg,
                          const uint16_t *values, uint8_t count);

#endif
