#include "PLCMod_RTD3CH.h"

// ================================================================
// RTD-3CH   typeId=1   swVersion=0x0100
//
// ModbusDevice: addr=32, FC4, startReg=3, count=3
//
// Local index layout:
//   0  RTD1   input reg   /100  C
//   1  RTD2   input reg   /100  C
//   2  RTD3   input reg   /100  C
//
// Version register: holding reg 0, read at boot via CheckModuleVersion().
// ================================================================

static const char* const _tempNames[] = { "RTD1", "RTD2", "RTD3" };
static const char* const _swNames[]   = { "Switch1", "Switch2" };

static const RegGroup_t _groups[] = {
    // groupName       regNames     type               start  count  scale    units  scanMode
    { "Temperatures",  _tempNames,  RTYPE_INPUT_REG,   0,     3,     100.0f,  "C",   SCAN_ALWAYS    },
    { "Switches",      _swNames,    RTYPE_DISCRETE,    0,     2,     1.0f,    "",    SCAN_ALWAYS    },
    { "Setpoints",     nullptr,     RTYPE_HOLDING_REG, 0,     3,     100.0f,  "C",   SCAN_ON_DEMAND },
};

const PLCModuleDesc_t RTD3CH_Desc = {
    .swVersion      = 0x0100,
    .versionRegIdx  = 0,
    .versionRegType = RTYPE_HOLDING_REG,
    .typeId         = 1,
    .name           = "RTD-3CH",
    .groups         = _groups,
    .groupCount     = 3,
    .defaultPollMs  = 5000,
};
