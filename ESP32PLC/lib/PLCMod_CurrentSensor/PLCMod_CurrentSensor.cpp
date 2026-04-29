#include "PLCMod_CurrentSensor.h"

// ================================================================
// CurrentSensor   typeId=3   swVersion=0x0100
//
// ModbusDevice: addr=5, FC4, startReg=1, count=5
//
// Local index layout:
//   0  DeviceType  raw (uint)
//   1  CurrentA    /100  A
//   2  CurrentB    /100  A
//   3  CurrentC    /100  A
//   4  (reserved)
// ================================================================

static const char* const _metaNames[]    = { "DeviceType" };
static const char* const _currentNames[] = { "CurrentA", "CurrentB", "CurrentC" };

static const RegGroup_t _groups[] = {
    // groupName   regNames        type              start  count  scale    units  scanMode
    { "Meta",      _metaNames,     RTYPE_INPUT_REG,  0,     1,     1.0f,    "",    SCAN_ALWAYS },
    { "Currents",  _currentNames,  RTYPE_INPUT_REG,  1,     3,     100.0f,  "A",   SCAN_ALWAYS },
};

const PLCModuleDesc_t CurrentSensor_Desc = {
    .swVersion      = 0x0100,
    .versionRegIdx  = 0,
    .versionRegType = RTYPE_HOLDING_REG,
    .typeId         = 3,
    .name           = "CurrentSensor",
    .groups         = _groups,
    .groupCount     = 2,
    .defaultPollMs  = 500,
};
