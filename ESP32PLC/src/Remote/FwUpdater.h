#pragma once
#include <Arduino.h>
#include <LittleFS.h>

#define FW_MAX_BIN_SIZE   30720   /* MBBP_APP_SIZE_MAX — 240 pages × 128 bytes */
#define FW_MAX_HEX_SIZE   65536   /* enough for any ATmega328P hex file */

class FwUpdater {
public:
    struct Status {
        volatile bool    running  = false;
        volatile bool    done     = false;
        volatile bool    success  = false;
        volatile uint8_t progress = 0;
        uint8_t          slaveId  = 0;
        char             message[64] = {};
    };

    FwUpdater(HardwareSerial &serial, int8_t dirPin);

    void init();  /* create /fw directory on LittleFS if absent */

    /*
     * Store uploaded firmware to LittleFS as /fw/<id>.bin.
     * isHex=true: data is Intel HEX text — converted to binary before saving.
     * isHex=false: data is raw binary — written directly.
     * Returns true on success.
     */
    bool storeFirmware(uint8_t slaveId, const uint8_t *data, size_t len, bool isHex);

    /* Start the OTA flash task (non-blocking, FreeRTOS task). */
    bool startUpdate(uint8_t slaveId);

    bool     hasFirmware(uint8_t slaveId);
    uint32_t firmwareSize(uint8_t slaveId);

    const Status &getStatus() const { return _status; }

    static String fwPath(uint8_t slaveId);

private:
    HardwareSerial &_serial;
    int8_t          _dirPin;
    Status          _status;
    TaskHandle_t    _taskHandle = nullptr;

    /* Intel HEX → binary. Returns true and sets *outSize on success. */
    static bool _hexToBin(const uint8_t *hex, size_t hexLen,
                           uint8_t *out, uint32_t *outSize);

    static void _progressCb(uint8_t pct);
    static void _updateTask(void *param);
};

extern FwUpdater fwUpdater;
