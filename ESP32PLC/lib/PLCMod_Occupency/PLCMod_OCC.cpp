#include "PLCMod_OCC.h"

// ================================================================
// OCC-TOL   typeId=2   swVersion=0x0100
//
// Sensor register map (from ComunicationUpdate in sensor firmware):
//
// INPUT REGISTERS (FC4):
//   0  OccStatus    0=vacant, 1=occupied
//   1  ZoneCount    active occupancy zones
//   2  VIN          supply voltage   × 100  (e.g. 1235 = 12.35 V)
//   3  Temp1        temperature 1    × 100  (e.g. 2315 = 23.15 °C)
//   4  Temp2        temperature 2    × 100
//   5  Humid1       humidity 1       × 100  (e.g. 5500 = 55.00 %)
//   6  Humid2       humidity 2       × 100
//   7  Pres         pressure (raw hPa)
//   8  Lux          light level (raw lux)
//
// COILS (FC1):
//   SENSOR_MOTION  motion detect (0=clear, 1=motion)
//
// HOLDING REGISTERS (FC3) — write path:
//   0  Command      SENSOR_CHANGE_ADDRESS | SENSOR_CHANGE_SCANRATE | SENSOR_SET_LED
//   1  LED_High     R byte + G high nibble
//   2  LED_Low      G low nibble + B byte
//
// LED write: use SetOcupyLED() in MasterController — queues FC16 to regs 0-2.
// Version:   holding reg 0 read at boot (deferred — CheckModuleVersion skips FC3).
// ================================================================

// ── Group 1: Occupancy status (raw, FC4 regs 0-1) ──────────────────────────
static const char* const _occNames[] = { "FWVer", "SenType" };

// ── Group 2: Environmental sensors (×100 scaling, FC4 regs 2-6) ────────────
static const char* const _envNames[] = { "VIN", "Temp1", "Temp2", "Humid1", "Humid2" };

// ── Group 3: Physical sensors (raw, FC4 regs 7-8) ──────────────────────────
static const char* const _physNames[] = { "Pres", "Lux" };

// ── Group 4: Motion coil (FC1 coil 0) ──────────────────────────────────────
static const char* const _motionNames[] = { "Motion" };

static const RegGroup_t _groups[] = {
    // groupName      regNames      type              startReg  count  scale    units   scanMode
    { "Occupancy",   _occNames,    RTYPE_INPUT_REG,  0,        2,     1.0f,   "",      SCAN_ALWAYS  },
    { "Environment", _envNames,    RTYPE_INPUT_REG,  2,        5,     100.0f, "",      SCAN_ALWAYS  },
    { "Physical",    _physNames,   RTYPE_INPUT_REG,  7,        2,     1.0f,   "",      SCAN_ALWAYS  },
    { "Motion",      _motionNames, RTYPE_COIL,       0,        1,     1.0f,   "",      SCAN_ALWAYS  },
};

const PLCModuleDesc_t OCC_Desc = {
    .swVersion      = 0x0100,
    .versionRegIdx  = 0,
    .versionRegType = RTYPE_HOLDING_REG,
    .typeId         = 2,
    .name           = "OCC-TOL",
    .groups         = _groups,
    .groupCount     = 4,
};
