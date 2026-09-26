#pragma once
#include <Arduino.h>
#include <LittleFS.h>

#define FW_MAX_BIN_SIZE   30720   /* MBBP_APP_SIZE_MAX — 240 pages × 128 bytes */
#define FW_MAX_HEX_SIZE   65536   /* enough for any ATmega328P hex file */

/* Stored images per slave: the normal application, and the ModBusBLUpdater
 * image used by the guided bootloader update. */
enum FwSlot : uint8_t {
    FW_SLOT_APP        = 0,   // /fw/<id>.bin
    FW_SLOT_BL_UPDATER = 1,   // /fw/<id>_blupd.bin
};

class FwUpdater {
public:
    struct Status {
        volatile bool    running  = false;
        volatile bool    done     = false;
        volatile bool    success  = false;
        volatile uint8_t progress = 0;
        uint8_t          slaveId  = 0;
        bool             blUpdate = false;   // true while a guided bootloader update runs
        char             message[64] = {};
    };

    FwUpdater(HardwareSerial &serial, int8_t dirPin);

    void init();  /* create /fw directory on LittleFS if absent */

    /*
     * Store uploaded firmware to LittleFS (path from fwPath()).
     * isHex=true: data is Intel HEX text — converted to binary before saving.
     * isHex=false: data is raw binary — written directly.
     * Returns true on success.
     */
    bool storeFirmware(uint8_t slaveId, const uint8_t *data, size_t len, bool isHex,
                       uint8_t slot = FW_SLOT_APP);

    /* Start the OTA flash task (non-blocking, FreeRTOS task).
     * skipTrigger=true skips the FC65 reboot command (device already in BL mode). */
    bool startUpdate(uint8_t slaveId, bool skipTrigger = false);

    /* Guided bootloader replacement (non-blocking). Needs both the updater and
     * the app image stored for this slave:
     *   1. flash the ModBusBLUpdater image like a normal app
     *   2. wait for the device to reboot into the new bootloader; find it by
     *      HELLO at its own address, else at the new bootloader's fallback (200)
     *   3. flash the app into the new bootloader
     *   4. if it came back at 200, move it back to its original address
     *   5. confirm the app answers at the original address                   */
    bool startBootloaderUpdate(uint8_t slaveId);

    bool     hasFirmware(uint8_t slaveId, uint8_t slot = FW_SLOT_APP);

    /* Bootloader version the stored updater image installs, read from the
     * 'MBBL' version record inside its embedded bootloader. 0 = none found. */
    uint8_t  updaterTargetVersion(uint8_t slaveId);
    static uint8_t findBlVersion(const uint8_t *buf, uint32_t len);
    uint32_t firmwareSize(uint8_t slaveId, uint8_t slot = FW_SLOT_APP);

    const Status &getStatus() const { return _status; }

    static String fwPath(uint8_t slaveId, uint8_t slot = FW_SLOT_APP);

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
    static void _blUpdateTask(void *param);
    void        _finish(bool ok, const char *msg);
};

extern FwUpdater fwUpdater;
