#ifndef MODBUS_BL_MASTER_H
#define MODBUS_BL_MASTER_H

/*
 * ModBusBLMaster — ESP32 MBBP master library.
 *
 * Workflow:
 *   1. triggerBootloader() — sends FC65 via normal ModBus to reboot slave.
 *   2. connect()           — sends MBBP HELLO, confirms bootloader is running.
 *   3. flashFirmware()     — streams firmware pages, verifies, jumps.
 *
 * Or use the combined helper:
 *   updateFirmware()       — does all three steps.
 *
 * Firmware must be raw binary (.bin from PlatformIO / avr-objcopy).
 * Pages are padded with 0xFF to fill the last 128-byte page.
 */

#include "Arduino.h"
#include "MBBP.h"

class ModBusBLMaster {
public:
    ModBusBLMaster(HardwareSerial &serial, int8_t dirPin = MBBP_DIR_PIN);

    void begin(long baud = 38400);

    /* Step 1: trigger bootloader via FC65. Returns true if ACK received. */
    bool triggerBootloader(uint8_t slaveId, uint32_t waitAfterMs = 300);

    /* Step 2: send MBBP HELLO, confirm bootloader is alive. */
    bool connect(uint8_t slaveId);

    /* Step 3: stream firmware binary, verify CRC, jump. */
    bool flashFirmware(uint8_t slaveId, const uint8_t *firmware, uint32_t firmwareSize);

    /* Convenience: trigger → connect → flash.
     * Set skipTrigger=true if the device is already in bootloader mode. */
    bool updateFirmware(uint8_t slaveId, const uint8_t *firmware,
                        uint32_t firmwareSize, uint8_t connectRetries = 5,
                        bool skipTrigger = false);

    void setProgressCallback(void (*cb)(uint8_t percent)) { _progressCb = cb; }
    const char *lastError() const { return _lastError; }

    /* Bootloader version from the most recent successful HELLO
     * (MBBP_BL_VERSION_LEGACY for bootloaders that don't report one). */
    uint8_t blVersion() const { return _blVersion; }

private:
    HardwareSerial &_serial;
    int8_t          _dirPin;
    void          (*_progressCb)(uint8_t percent);
    const char     *_lastError;
    uint8_t         _blVersion = 0;

    void     _txEnable();
    void     _txDisable();
    uint16_t _crc16(const uint8_t *buf, uint16_t len);

    void    _sendFrame(uint8_t addr, uint8_t cmd,
                       const uint8_t *data, uint16_t dataLen);
    uint8_t _recvFrame(uint8_t expectedAddr, uint8_t *outData,
                       uint16_t *outLen, uint32_t timeoutMs = 2000);
    /* Send a frame and wait for expectRsp, resending on timeout/garbled reply.
       Only for idempotent commands (WRITE_PAGE, VERIFY). */
    bool    _transact(uint8_t addr, uint8_t cmd, const uint8_t *data, uint16_t dataLen,
                      uint8_t expectRsp, uint8_t *outData, uint16_t *outLen,
                      uint32_t timeoutMs);

    uint16_t _modbusCalcCRC(const uint8_t *buf, uint8_t len);
    bool     _sendFC65(uint8_t slaveId);

    void _setError(const char *msg) { _lastError = msg; }
    char _errBuf[64] = {};            /* backing store for formatted errors */
    void _setErrorf(const char *fmt, ...);
    static const char *_errName(uint8_t code);
};

#endif /* MODBUS_BL_MASTER_H */
