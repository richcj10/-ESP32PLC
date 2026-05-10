#include "PLCMod_OCC.h"

// ================================================================
// OCC-TOL   typeId=2   swVersion=0x0100
//
// Occupancy / Time-on-Light sensor module.
//
// ModbusDevice: addr=17, FC4, startReg=0, count=3
//
// Local index layout (INPUT_REGISTERS, FC4):
//   0  OccStatus   0=vacant, 1=occupied
//   1  ZoneCount   number of active occupancy zones
//   2  TimeOnSec   seconds device has been in current state
//
// LED control is a WRITE path via SetOcupyLED() in MasterController.
// Version register: holding reg 0, read at boot via CheckModuleVersion().
// ================================================================

static const char* const _occNames[]  = { "OccStatus", "ZoneCount", "TimeOnSec" };

static const RegGroup_t _groups[] = {
    // groupName    regNames    type              start  count  scale   units  scanMode
    { "Occupancy", _occNames,  RTYPE_INPUT_REG,  0,     3,     1.0f,   "",    SCAN_ALWAYS    },
};

const PLCModuleDesc_t OCC_Desc = {
    .swVersion      = 0x0100,
    .versionRegIdx  = 0,
    .versionRegType = RTYPE_HOLDING_REG,
    .typeId         = 2,
    .name           = "OCC-TOL",
    .groups         = _groups,
    .groupCount     = 1,
    .defaultPollMs  = 500,
};
