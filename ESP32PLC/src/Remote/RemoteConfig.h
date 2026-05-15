#ifndef REMOTECONFIG_H
#define REMOTECONFIG_H

#include <stdint.h>

// Forward declaration — PLCModuleInstance_t holds a pointer only.
// Files that call ModbusDevice methods must include ModbusDevice.h directly.
class ModbusDevice;

// ================================================================
// PLC Module Descriptor System
//
// Two-layer model:
//   PLCModuleDesc_t  — compile-time const, describes a module TYPE
//                      (register layout, names, scale, version)
//   PLCModuleInstance_t — runtime, one per physical device on the bus
//                         (descriptor + ModbusDevice + live status)
//
// Only module types listed in PLCModuleRegistry[] consume flash.
// ================================================================

// ----------------------------------------------------------------
// Enums
// ----------------------------------------------------------------

typedef enum : uint8_t {
    SCAN_ALWAYS,       // polled every device poll cycle
    SCAN_ON_DEMAND,    // read only when explicitly requested
} ScanMode_t;

typedef enum : uint8_t {
    RTYPE_INPUT_REG    = 4,   // FC4  read-only sensor registers
    RTYPE_HOLDING_REG  = 3,   // FC3  read/write config / setpoints
    RTYPE_COIL         = 1,   // FC1  digital outputs (read)
    RTYPE_DISCRETE     = 2,   // FC2  digital inputs / switches
} RegType_t;

// Guard matches the identical definition in the ESP32ModbusMaster library
// (RemoteDeviceConfig.h). Whichever header is included first defines the type.
#ifndef MODULE_STATUS_T_DEFINED
#define MODULE_STATUS_T_DEFINED
typedef enum : uint8_t {
    MODULE_UNKNOWN  = 0,   // version check not yet run
    MODULE_VALID,          // version matched  — data is trusted and published
    MODULE_INVALID,        // version mismatch — data suppressed until resolved
    MODULE_OFFLINE,        // not responding on the bus
} ModuleStatus_t;
#endif

// ----------------------------------------------------------------
// RegGroup_t — a contiguous block of same-type registers
//
// startReg is a LOCAL index (0 = first register the ModbusDevice
// reads from the remote device, not the raw Modbus address).
//
// Engineering value = (float)raw / scale
//   e.g. scale=100.0  →  raw 2315 becomes 23.15 °C
//        scale=1.0    →  raw passed through unchanged (coils, dirs)
//
// regNames: per-register label array, or nullptr for "Reg[N]" fallback.
// Array must have at least `count` entries if non-null.
// ----------------------------------------------------------------
typedef struct {
    const char*        groupName;
    const char* const* regNames;    // nullable — generic fallback if null
    RegType_t          type;
    uint8_t            startReg;    // local index into ModbusDevice reg array
    uint8_t            count;
    float              scale;       // eng = raw / scale
    const char*        units;       // display string: "C", "A", "deg", ""
    ScanMode_t         scanMode;
} RegGroup_t;

// ----------------------------------------------------------------
// PLCModuleDesc_t — compile-time const, one per module TYPE
//
// swVersion must match the version register on the remote device.
// A mismatch at boot sets status = MODULE_INVALID and suppresses
// all data publishing for that instance until resolved.
//
// versionRegIdx: local index of the register that holds the version.
// versionRegType: FC used to read it (usually RTYPE_HOLDING_REG).
// ----------------------------------------------------------------
typedef struct {
    uint16_t           swVersion;       // SW version this descriptor targets
    uint8_t            versionRegIdx;   // local index of version register
    RegType_t          versionRegType;  // FC for the version register read
    uint8_t            typeId;          // unique module type ID (never reuse)
    const char*        name;            // human label: "RTD-3CH", "WeatherStation"
    const RegGroup_t*  groups;          // array of register group descriptors
    uint8_t            groupCount;
} PLCModuleDesc_t;

// ----------------------------------------------------------------
// PLCModuleInstance_t — runtime state for one physical device
// ----------------------------------------------------------------
typedef struct {
    const PLCModuleDesc_t* desc;        // points to static type descriptor
    ModbusDevice*          device;      // Modbus driver for this device
    uint8_t                address;     // Modbus bus address
    ModuleStatus_t         status;      // live validation status
} PLCModuleInstance_t;

// ----------------------------------------------------------------
// Global registry — all module types compiled into this build
// Only types listed here occupy flash.
// ----------------------------------------------------------------
extern const PLCModuleDesc_t* const PLCModuleRegistry[];
extern const uint8_t                PLCModuleRegistryCount;

// Look up a descriptor by typeId; returns nullptr if not compiled in.
const PLCModuleDesc_t* RegistryFind(uint8_t typeId);

// Version check — call once after device comes online.
// Reads versionReg from the device and compares to desc->swVersion.
// Sets inst->status to MODULE_VALID or MODULE_INVALID.
void CheckModuleVersion(PLCModuleInstance_t* inst);

// Helper: resolved register name ("Water Inlet" or "Reg[0]" fallback).
const char* RegGroupGetName(const RegGroup_t* group, uint8_t idx);

// Module type descriptors live in lib/PLCMod_<name>/.
// Include the relevant header there, then add to PLCModuleRegistry[].

#endif // REMOTECONFIG_H
