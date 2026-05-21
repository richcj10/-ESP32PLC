#ifndef DIGITAL_H
#define DIGITAL_H

#include <stdint.h>

/* ── MCU fixed-pin init ──────────────────────────────────────────────────── */
void DigitalStart(void);        // LED, USER_SW — always present

/* ── Shield I/O init (call after QueryLocalDevice) ───────────────────────── */
void IOStart(void);             // pins from DeviceType; re-entrant safe

/* ── Main-loop scan ──────────────────────────────────────────────────────── */
void ScanIO(void);              // digitalRead all registered inputs

/* ── Accessors ───────────────────────────────────────────────────────────── */
uint8_t GetInputCount(void);
uint8_t GetOutputCount(void);
bool    GetInput(uint8_t n);    // 0-based; false if n out of range
bool    GetOutput(uint8_t n);   // reads back output register
void    SetOutput(uint8_t n, bool val);

/* ── MCU user controls ───────────────────────────────────────────────────── */
void SetUserLED(bool val);
void ToggletUserLED(void);      // legacy spelling kept — Functions.cpp uses it
bool GetUserSWValue(void);
void ScanUserInput(void);

/* ── Legacy shim ─────────────────────────────────────────────────────────── */
void ScanArrayAdd(char pin);    // no-op; keeps DeviceConfig.cpp compiling

#endif
