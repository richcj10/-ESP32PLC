#pragma once
#include "Modules/Module.h"
#include <stdint.h>

#define FW_MOD_MAX_DEVICES  2
#define FW_MOD_MAX_STEPS    40

// Modbus constants for HighPowerOutput (typeId=40)
#define HPO_DEVICE_TYPE        40
#define HPO_MODE_CMD_ENTER_FW  0xFAF0u
#define HPO_MODE_CMD_EXIT_FW   0x0FAFu
#define HPO_COIL_FIRE_BASE     2    // coils 2-11 = outputs 1-10
#define HPO_OUTPUTS_PER_DEV    10
#define HPO_REG_SAFETY_V       0    // offset within the "Power" group (startReg=2): SafetyV at offset 0, VIN at offset 1
#define HPO_POWER_START_REG    2    // Modbus startReg of the Power polling group
#define HPO_SAFETY_V_MIN       10.0f // volts — matches SAFETY_THRESHOLD in HPO firmware

class ModbusDevice;

class FireworksModule : public Module {
public:
    const char* name() override { return "fireworks"; }

    void begin()  override;
    void update() override;

    uint32_t updateIntervalMs() override { return 10; }

    void getLiveData(JsonObject& out) override;
    void registerRoutes(AsyncWebServer& svr) override;

private:
    struct Device {
        uint8_t       addr;
        char          name[24];
        ModbusDevice* pollDev;  // linked polling group for register reads
    };

    struct Step {
        uint8_t  devIdx;    // index into _devs[]
        uint8_t  output;    // 1-10
        uint32_t delayMs;   // wait after previous step before firing this one
    };

    Device  _devs[FW_MOD_MAX_DEVICES];
    uint8_t _devCount = 0;

    Step     _seq[FW_MOD_MAX_STEPS];
    uint8_t  _seqLen     = 0;
    uint8_t  _seqStep    = 0;
    bool     _seqRunning = false;
    uint32_t _seqLastMs  = 0;

    float _getSafetyV(uint8_t devIdx)   const;
    bool  _isSafetyOk(uint8_t devIdx)  const;
    bool  _allSafetyOk()               const;
    void  _startSequence();

    void _fireOutput(uint8_t addr, uint8_t output);
    void _sendModeCmd(uint8_t addr, uint16_t cmd);
    void _abortSequence();

    bool _inputTriggerEnabled = false;
    bool _inputTrigLastState  = false;  // edge detect — fire on rising edge of IN0
};

extern FireworksModule fwModule;
