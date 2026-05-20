#ifndef REMOTECONFIG_H
#define REMOTECONFIG_H

// Guard matches the identical definition in the ESP32ModbusMaster library
// (RemoteDeviceConfig.h). Whichever header is included first defines the type.
#ifndef MODULE_STATUS_T_DEFINED
#define MODULE_STATUS_T_DEFINED
typedef enum : uint8_t {
    MODULE_UNKNOWN        = 0,  // version check not yet run
    MODULE_VALID,               // version + typeId matched — data trusted
    MODULE_INVALID,             // version mismatch — data suppressed
    MODULE_OFFLINE,             // not responding on bus
    MODULE_TYPE_MISMATCH,       // version OK but typeId mismatch — data suppressed
} ModuleStatus_t;
#endif

#endif // REMOTECONFIG_H
