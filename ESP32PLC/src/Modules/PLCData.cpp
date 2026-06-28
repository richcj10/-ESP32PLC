#include "PLCData.h"
#include <math.h>
#include <string.h>
#include "Remote/MasterController.h"
#include "FileSystem/FSInterface.h"

float plcReadRaw(uint8_t addr, uint8_t regIndex) {
    if (addr == 0xFF) return NAN;
    if (!master.findByAddress(addr)) return NAN;
    const RemoteConfig_t& cfg = GetRemoteConfig();
    uint8_t flat = 0;
    for (uint8_t di = 0; di < cfg.deviceCount; di++) {
        if (cfg.devices[di].address != addr) continue;
        for (uint8_t gi = 0; gi < cfg.devices[di].groupCount; gi++) {
            uint8_t cnt = cfg.devices[di].groups[gi].count;
            if (regIndex < flat + cnt) {
                for (uint8_t pi = 0; pi < RemoteGrpCount(); pi++) {
                    if (RemoteGrpDevIdx(pi) == di && RemoteGrpGrpIdx(pi) == gi) {
                        ModbusDevice* d = RemoteGrpDevice(pi);
                        if (!d) return NAN;
                        return (float)d->getRaw(regIndex - flat);
                    }
                }
                return NAN;
            }
            flat += cnt;
        }
        break;
    }
    return NAN;
}

uint8_t plcAddress(const char* deviceName) {
    const RemoteConfig_t& cfg = GetRemoteConfig();
    for (uint8_t i = 0; i < cfg.deviceCount; i++)
        if (strcasecmp(cfg.devices[i].name, deviceName) == 0)
            return cfg.devices[i].address;
    return 0xFF;
}

uint8_t plcRegIndex(uint8_t addr, const char* regName) {
    const RemoteConfig_t& cfg = GetRemoteConfig();
    for (uint8_t di = 0; di < cfg.deviceCount; di++) {
        if (cfg.devices[di].address != addr) continue;
        uint8_t flat = 0;
        for (uint8_t gi = 0; gi < cfg.devices[di].groupCount; gi++) {
            const auto& grp = cfg.devices[di].groups[gi];
            for (uint8_t ri = 0; ri < grp.count; ri++) {
                if (grp.regs[ri][0] && strcasecmp(grp.regs[ri], regName) == 0)
                    return flat;
                flat++;
            }
        }
        break;
    }
    return 0xFF;
}

float plcRead(const char* deviceName, const char* regName) {
    uint8_t addr = plcAddress(deviceName);
    if (addr == 0xFF) return NAN;
    uint8_t reg = plcRegIndex(addr, regName);
    if (reg == 0xFF) return NAN;
    return plcReadRaw(addr, reg);
}

void plcWrite(uint8_t addr, uint16_t reg, uint16_t value) {
    master.queueWrite(addr, reg, value);
}

void plcWriteMulti(uint8_t addr, uint16_t startReg, const uint16_t* values, uint8_t count) {
    master.queueWriteMulti(addr, startReg, values, count);
}
