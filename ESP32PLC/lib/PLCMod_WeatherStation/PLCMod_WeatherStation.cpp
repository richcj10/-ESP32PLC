#include "PLCMod_WeatherStation.h"

// ================================================================
// WeatherStation   typeId=2   swVersion=0x0100
//
// ModbusDevice: addr=48, FC4, startReg=0, count=7
//
// Local index layout:
//   0  DeviceType   raw (uint)
//   1  OutsideTemp  /100  C
//   2  Humidity     /100  %RH
//   3  Moisture     /100
//   4  WindSpeed    /100  m/s
//   5  WindDir      raw   deg   (0-359, no divide)
//   6  Rain         /100  mm
//
// Groups split wherever scale changes so each group has one scale.
// ================================================================

static const char* const _metaNames[]  = { "DeviceType" };
static const char* const _envNames[]   = { "OutsideTemp", "Humidity", "Moisture" };
static const char* const _wspdNames[]  = { "WindSpeed" };
static const char* const _wdirNames[]  = { "WindDir" };
static const char* const _rainNames[]  = { "Rain" };

static const RegGroup_t _groups[] = {
    // groupName     regNames      type              start  count  scale    units   scanMode
    { "Meta",        _metaNames,   RTYPE_INPUT_REG,  0,     1,     1.0f,    "",     SCAN_ALWAYS },
    { "Environment", _envNames,    RTYPE_INPUT_REG,  1,     3,     100.0f,  "",     SCAN_ALWAYS },
    { "WindSpeed",   _wspdNames,   RTYPE_INPUT_REG,  4,     1,     100.0f,  "m/s",  SCAN_ALWAYS },
    { "WindDir",     _wdirNames,   RTYPE_INPUT_REG,  5,     1,     1.0f,    "deg",  SCAN_ALWAYS },
    { "Rain",        _rainNames,   RTYPE_INPUT_REG,  6,     1,     100.0f,  "mm",   SCAN_ALWAYS },
};

const PLCModuleDesc_t WeatherStation_Desc = {
    .swVersion      = 0x0100,
    .versionRegIdx  = 0,
    .versionRegType = RTYPE_HOLDING_REG,
    .typeId         = 2,
    .name           = "WeatherStation",
    .groups         = _groups,
    .groupCount     = 5,
    .defaultPollMs  = 1000,
};
