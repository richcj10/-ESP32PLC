#pragma once
#include <stdint.h>

// ── Level 1: raw register read — O(1) after begin() ─────────────────────────
float   plcReadRaw(uint8_t addr, uint8_t regIndex);

// ── Level 2: name → index resolution — O(n), cache result at begin() ────────
uint8_t plcAddress(const char* deviceName);               // "WeatherStation" → 0x16
uint8_t plcRegIndex(uint8_t addr, const char* regName);   // addr + "Temp"    → 2

// ── Convenience: by-name every call — O(n), fine for one-offs ───────────────
float   plcRead(const char* deviceName, const char* regName);

// ── Writes (queued, executed on next Modbus poll) ────────────────────────────
void    plcWrite(uint8_t addr, uint16_t reg, uint16_t value);
void    plcWriteMulti(uint8_t addr, uint16_t startReg, const uint16_t* values, uint8_t count);
