#include "RemoteConfig.h"
#include <ModbusDevice.h>
#include "Devices/Log.h"
#include <stdio.h>

// ================================================================
// Enabled modules — include the lib header, add a pointer to the
// registry array below.  That is the only change needed to
// add or remove a module from this build.
// ================================================================

#include "PLCMod_OCC.h"

const PLCModuleDesc_t* const PLCModuleRegistry[] = {
    &OCC_Desc,
};
const uint8_t PLCModuleRegistryCount =
    sizeof(PLCModuleRegistry) / sizeof(PLCModuleRegistry[0]);

// ================================================================
// Registry helpers
// ================================================================

const PLCModuleDesc_t* RegistryFind(uint8_t typeId) {
    for (uint8_t i = 0; i < PLCModuleRegistryCount; i++) {
        if (PLCModuleRegistry[i]->typeId == typeId) return PLCModuleRegistry[i];
    }
    return nullptr;
}

const char* RegGroupGetName(const RegGroup_t* group, uint8_t idx) {
    if (group->regNames && idx < group->count) return group->regNames[idx];
    static char fallback[12];
    snprintf(fallback, sizeof(fallback), "Reg[%u]", idx);
    return fallback;
}

// ================================================================
// Version check — call once when a device first comes online.
//
// Reads the version register from the already-polled register array
// and compares to desc->swVersion.  Sets inst->status to
// MODULE_VALID or MODULE_INVALID and logs the result.
// ================================================================

void CheckModuleVersion(PLCModuleInstance_t* inst) {
    if (!inst || !inst->desc || !inst->device) return;

    const PLCModuleDesc_t* desc = inst->desc;

    // Version register is only readable if it lives in the same FC block
    // the ModbusDevice is already polling (RTYPE_INPUT_REG / FC4).
    // Holding-register version reads require a separate one-shot packet
    // that is not yet wired up — mark valid and defer.
    if (desc->versionRegType != RTYPE_INPUT_REG) {
        inst->status = MODULE_VALID;
        Log(LOG, "[%s] addr=%u version check deferred (holding reg not polled)\r\n",
            desc->name, inst->address);
        return;
    }

    uint16_t remoteVer = inst->device->getRaw(desc->versionRegIdx);

    if (remoteVer == desc->swVersion) {
        inst->status = MODULE_VALID;
        Log(LOG, "[%s] addr=%u version OK (0x%04X)\r\n",
            desc->name, inst->address, remoteVer);
    } else {
        inst->status = MODULE_INVALID;
        Log(ERROR, "[%s] addr=%u VERSION MISMATCH expected=0x%04X got=0x%04X data suppressed\r\n",
            desc->name, inst->address, desc->swVersion, remoteVer);
    }
}
