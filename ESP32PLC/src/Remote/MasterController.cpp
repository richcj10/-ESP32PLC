#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>
#include "Devices/Log.h"
#include "MQTT.h"

#include "PLCMod_RTD3CH.h"
#include "PLCMod_WeatherStation.h"
#include "PLCMod_CurrentSensor.h"

// ----------------------------------------------------------------
// Low-level Modbus device drivers
// ----------------------------------------------------------------
ModbusDevice tempSensor   (32, READ_INPUT_REGISTERS, 3, 3);
ModbusDevice weatherSensor(48, READ_INPUT_REGISTERS, 0, 7);
ModbusDevice currentSensor( 5, READ_INPUT_REGISTERS, 1, 5);

ModbusMasterController master(Serial1, 38400, 16, 17, 20);

// ----------------------------------------------------------------
// Module instances — descriptor + device + runtime status
// ----------------------------------------------------------------
PLCModuleInstance_t tempInst    = { &RTD3CH_Desc,         &tempSensor,    32, MODULE_UNKNOWN };
PLCModuleInstance_t weatherInst = { &WeatherStation_Desc, &weatherSensor, 48, MODULE_UNKNOWN };
PLCModuleInstance_t currentInst = { &CurrentSensor_Desc,  &currentSensor,  5, MODULE_UNKNOWN };

// ----------------------------------------------------------------
// Comms status MQTT helper
// ----------------------------------------------------------------
static void publishDeviceStatus(const char* name, ModbusDevice* d, bool online) {
    char buf[48];
    if (online) {
        snprintf(buf, sizeof(buf), "online");
    } else {
        snprintf(buf, sizeof(buf), "offline | errors: %lu", d->failedRequests());
    }
    char topic[40];
    snprintf(topic, sizeof(topic), "Devices/%s/comms", name);
    Log(online ? LOG : NOTIFY, "Device %s %s\r\n", name, buf);
    if (GetMQTTStatus()) GetMQTTClient().publish(topic, buf);
}

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
char RemoteStart() {
    tempSensor.setPollRate(5000);
    weatherSensor.setPollRate(1000);
    currentSensor.setPollRate(500);

    // --- Status change: version check on first online, mark offline on loss ---
    tempSensor.onStatusChange([](ModbusDevice* d, bool online) {
        if (online) CheckModuleVersion(&tempInst);
        else        tempInst.status = MODULE_OFFLINE;
        publishDeviceStatus("TempSensor", d, online);
    });

    weatherSensor.onStatusChange([](ModbusDevice* d, bool online) {
        if (online) CheckModuleVersion(&weatherInst);
        else        weatherInst.status = MODULE_OFFLINE;
        publishDeviceStatus("WeatherSensor", d, online);
    });

    currentSensor.onStatusChange([](ModbusDevice* d, bool online) {
        if (online) CheckModuleVersion(&currentInst);
        else        currentInst.status = MODULE_OFFLINE;
        publishDeviceStatus("CurrentSensor", d, online);
    });

    // --- Data received: only log if module is valid ---
    weatherSensor.onDataReceived([](ModbusDevice* d) {
        if (weatherInst.status != MODULE_VALID) return;
        Log(DEBUG, "WX ok | req:%lu ok:%lu fail:%lu\r\n",
            d->totalRequests(), d->successRequests(), d->failedRequests());
        Log(DEBUG, "WX type=%u temp=%.2f humid=%.2f mC=%.2f wind=%.2f dir=%u rain=%.2f\r\n",
            d->getRaw(0), d->getFloat(1), d->getFloat(2),
            d->getFloat(3), d->getFloat(4), d->getRaw(5), d->getFloat(6));
    });

    currentSensor.onDataReceived([](ModbusDevice* d) {
        if (currentInst.status != MODULE_VALID) return;
        Log(DEBUG, "CS ok | req:%lu ok:%lu fail:%lu\r\n",
            d->totalRequests(), d->successRequests(), d->failedRequests());
        Log(DEBUG, "CS type=%u A=%.2f B=%.2f C=%.2f\r\n",
            d->getRaw(0), d->getFloat(1), d->getFloat(2), d->getFloat(3));
    });

    master.addDevice(tempSensor);
    master.addDevice(weatherSensor);
    master.addDevice(currentSensor);
    master.begin();
    return 1;
}

// ----------------------------------------------------------------
// Runtime
// ----------------------------------------------------------------
void RemoteRun() {
    master.update();

    static unsigned long lastStatDump = 0;
    if (millis() - lastStatDump > 10000) {
        lastStatDump = millis();
        Log(DEBUG, "WX req:%lu ok:%lu fail:%lu | CS req:%lu ok:%lu fail:%lu\r\n",
            weatherSensor.totalRequests(), weatherSensor.successRequests(), weatherSensor.failedRequests(),
            currentSensor.totalRequests(),  currentSensor.successRequests(),  currentSensor.failedRequests());
    }
}

// ----------------------------------------------------------------
// GetModuleValue — scale raw register using the group descriptor
// ----------------------------------------------------------------
float GetModuleValue(const PLCModuleInstance_t* inst, uint8_t localIdx) {
    if (!inst || inst->status != MODULE_VALID || !inst->desc || !inst->device) return 0.0f;

    const PLCModuleDesc_t* desc = inst->desc;
    for (uint8_t g = 0; g < desc->groupCount; g++) {
        const RegGroup_t* grp = &desc->groups[g];
        uint8_t end = grp->startReg + grp->count;
        if (localIdx < grp->startReg || localIdx >= end) continue;

        if (grp->type == RTYPE_COIL || grp->type == RTYPE_DISCRETE) {
            return inst->device->getCoil(localIdx) ? 1.0f : 0.0f;
        }
        if (grp->scale == 1.0f) return (float)inst->device->getRaw(localIdx);
        return (float)inst->device->getSigned(localIdx) / grp->scale;
    }
    return 0.0f;
}

// ----------------------------------------------------------------
// LogModuleData — prints all SCAN_ALWAYS groups using descriptor names
// ----------------------------------------------------------------
void LogModuleData(const PLCModuleInstance_t* inst) {
    if (!inst || !inst->desc) return;

    const PLCModuleDesc_t* desc = inst->desc;
    const char* statusStr =
        inst->status == MODULE_VALID   ? "VALID"   :
        inst->status == MODULE_INVALID ? "INVALID" :
        inst->status == MODULE_OFFLINE ? "OFFLINE" : "UNKNOWN";

    Log(LOG, "[%s addr=%u] %s\r\n", desc->name, inst->address, statusStr);
    if (inst->status != MODULE_VALID) return;

    for (uint8_t g = 0; g < desc->groupCount; g++) {
        const RegGroup_t* grp = &desc->groups[g];
        if (grp->scanMode == SCAN_ON_DEMAND) continue;
        for (uint8_t r = 0; r < grp->count; r++) {
            float val = GetModuleValue(inst, grp->startReg + r);
            Log(LOG, "  %-14s = %.2f %s\r\n", RegGroupGetName(grp, r), val, grp->units);
        }
    }
}

// ----------------------------------------------------------------
// Diagnostic helpers — now driven by descriptor
// ----------------------------------------------------------------
char ReadRemoteTemp()    { LogModuleData(&tempInst);    return 1; }
char ReadRemoteWeather() { LogModuleData(&weatherInst); return 1; }
char ReadRemoteCurrent() { LogModuleData(&currentInst); return 1; }

// ----------------------------------------------------------------
// GetRemoteDataFromQue — legacy API for MQTT.cpp
// Returns 0 if the module is not MODULE_VALID.
// ----------------------------------------------------------------
float GetRemoteDataFromQue(unsigned char x, bool Divide) {
    if (x >= 1  && x <= 3) {
        if (tempInst.status != MODULE_VALID) return 0.0f;
        return Divide ? tempSensor.getFloat(x - 1) : (float)tempSensor.getRaw(x - 1);
    }
    if (x >= 4  && x <= 10) {
        if (weatherInst.status != MODULE_VALID) return 0.0f;
        return Divide ? weatherSensor.getFloat(x - 4) : (float)weatherSensor.getRaw(x - 4);
    }
    if (x >= 12 && x <= 16) {
        if (currentInst.status != MODULE_VALID) return 0.0f;
        return Divide ? currentSensor.getFloat(x - 12) : (float)currentSensor.getRaw(x - 12);
    }
    return 0.0f;
}

// ----------------------------------------------------------------
// Poll rate control
// ----------------------------------------------------------------
void SetTempPollRate   (unsigned long ms) { tempSensor.setPollRate(ms);    }
void SetWeatherPollRate(unsigned long ms) { weatherSensor.setPollRate(ms); }
void SetCurrentPollRate(unsigned long ms) { currentSensor.setPollRate(ms); }

// ----------------------------------------------------------------
// Write helpers
// ----------------------------------------------------------------
char SetOcupyLED(int Address, unsigned char R, unsigned char G, unsigned char B) {
    uint16_t vals[3] = { 12, R, (uint16_t)((G << 8) | B) };
    return master.queueWriteMulti((uint8_t)Address, 0x02, vals, 3) ? 1 : 0;
}

// ----------------------------------------------------------------
// Stubs
// ----------------------------------------------------------------
int   ReadDeviceType(int Address) { return 0; }
float readDeviceVIN (int Address) { return -1; }
char  GetRemoteTemp (char TempCH) { return 0; }
