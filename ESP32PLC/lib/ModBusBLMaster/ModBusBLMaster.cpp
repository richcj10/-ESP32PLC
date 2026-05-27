#include "ModBusBLMaster.h"
#include "Devices/Log.h"

#define RSP_BUF_SIZE (MBBP_MAX_PAYLOAD + 4)

ModBusBLMaster::ModBusBLMaster(HardwareSerial &serial, int8_t dirPin)
    : _serial(serial), _dirPin(dirPin), _progressCb(nullptr),
      _lastError("none") {}

void ModBusBLMaster::begin(long baud) {
    _serial.begin(baud, SERIAL_8N1);
    if (_dirPin >= 0) {
        pinMode(_dirPin, OUTPUT);
        digitalWrite(_dirPin, LOW);
    }
    delay(10);
}

/* ── Direction control ──────────────────────────────────────────────────── */

void ModBusBLMaster::_txEnable() {
    if (_dirPin >= 0) digitalWrite(_dirPin, HIGH);
}

void ModBusBLMaster::_txDisable() {
    if (_dirPin >= 0) digitalWrite(_dirPin, LOW);
}

/* ── CRC-16 (Modbus: poly 0xA001, init 0xFFFF) ──────────────────────────── */

uint16_t ModBusBLMaster::_crc16(const uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    return crc;
}

uint16_t ModBusBLMaster::_modbusCalcCRC(const uint8_t *buf, uint8_t len) {
    return _crc16(buf, len);
}

/* ── MBBP frame I/O ─────────────────────────────────────────────────────── */

void ModBusBLMaster::_sendFrame(uint8_t addr, uint8_t cmd,
                                const uint8_t *data, uint16_t dataLen)
{
    uint8_t hdr[4] = { addr, cmd,
                       (uint8_t)(dataLen >> 8),
                       (uint8_t)(dataLen & 0xFF) };

    /* CRC over header + data */
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < 4; i++) {
        crc ^= hdr[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    for (uint16_t i = 0; i < dataLen; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }

    while (_serial.available()) _serial.read();

    _txEnable();
    delayMicroseconds(200);
    _serial.write(hdr, 4);
    if (dataLen > 0) _serial.write(data, dataLen);
    _serial.write((uint8_t)(crc >> 8));
    _serial.write((uint8_t)(crc & 0xFF));
    _serial.flush();
    delayMicroseconds(500);
    _txDisable();
}

uint8_t ModBusBLMaster::_recvFrame(uint8_t expectedAddr, uint8_t *outData,
                                    uint16_t *outLen, uint32_t timeoutMs)
{
    uint8_t  buf[RSP_BUF_SIZE];
    uint8_t  len   = 0;
    uint32_t start = millis();

    while (!_serial.available()) {
        if (millis() - start > timeoutMs) { _setError("recv timeout"); return 0; }
    }

    uint32_t last_byte = millis();
    while ((millis() - last_byte) < 3) {
        if (_serial.available() && len < RSP_BUF_SIZE) {
            buf[len++] = (uint8_t)_serial.read();
            last_byte  = millis();
        }
        if (millis() - start > timeoutMs + 100) break;
    }

    if (len < 6)              { _setError("frame too short");      return 0; }
    if (buf[0] != expectedAddr){ _setError("wrong address");        return 0; }

    uint8_t  cmd     = buf[1];
    uint16_t pay_len = ((uint16_t)buf[2] << 8) | buf[3];

    if (len != (uint8_t)(4 + pay_len + 2)) { _setError("frame length mismatch"); return 0; }

    uint16_t rx_crc   = ((uint16_t)buf[len - 2] << 8) | buf[len - 1];
    uint16_t calc_crc = _crc16(buf, len - 2);
    if (rx_crc != calc_crc) { _setError("CRC mismatch"); return 0; }

    if (outData && pay_len > 0)
        memcpy(outData, buf + 4, pay_len);
    if (outLen) *outLen = pay_len;

    return cmd;
}

/* ── FC65 trigger ───────────────────────────────────────────────────────── */
/*
 * Frame: [ADDR][0x41][0xB0][0x07][CRC_lo][CRC_hi]
 * SimpleModbusSlave stores CRC as [lo, hi] (low byte first).
 */
bool ModBusBLMaster::_sendFC65(uint8_t slaveId) {
    uint8_t frame[6];
    frame[0] = slaveId;
    frame[1] = MBBP_FC_TRIGGER;
    frame[2] = MBBP_MAGIC_H;
    frame[3] = MBBP_MAGIC_L;

    uint16_t crc = _crc16(frame, 4);
    frame[4] = (uint8_t)(crc & 0xFF); /* lo byte first */
    frame[5] = (uint8_t)(crc >> 8);

    while (_serial.available()) _serial.read();

    Log(NOTIFY, "[BL] FC65 → slave 0x%02X  frame: %02X %02X %02X %02X %02X %02X\r\n",
        slaveId, frame[0], frame[1], frame[2], frame[3], frame[4], frame[5]);

    _txEnable();
    delayMicroseconds(200);
    _serial.write(frame, 6);
    _serial.flush();
    delayMicroseconds(500);
    _txDisable();

    uint32_t start = millis();
    uint8_t  rsp[6];
    uint8_t  rlen = 0;
    while ((millis() - start) < 500) {
        if (_serial.available() && rlen < 6)
            rsp[rlen++] = (uint8_t)_serial.read();
        if (rlen == 6) break;
    }

    Log(NOTIFY, "[BL] FC65 ACK: got %u bytes in %lums\r\n",
        (unsigned)rlen, (unsigned long)(millis() - start));
    for (uint8_t i = 0; i < rlen; i++)
        Log(NOTIFY, "  [%u] 0x%02X\r\n", i, rsp[i]);

    if (rlen < 6) {
        Log(ERROR, "[BL] FC65 FAIL: no ACK (expected 6, got %u)\r\n", (unsigned)rlen);
        _setError("FC65 no ACK"); return false;
    }
    if (rsp[0] != slaveId || rsp[1] != MBBP_FC_TRIGGER) {
        Log(ERROR, "[BL] FC65 FAIL: bad ACK addr=0x%02X cmd=0x%02X\r\n", rsp[0], rsp[1]);
        _setError("FC65 bad ACK"); return false;
    }
    Log(NOTIFY, "[BL] FC65 OK — slave acknowledged trigger\r\n");
    return true;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

bool ModBusBLMaster::triggerBootloader(uint8_t slaveId, uint32_t waitAfterMs) {
    Log(NOTIFY, "[BL] triggerBootloader slave=%u waitAfter=%lums\r\n",
        (unsigned)slaveId, (unsigned long)waitAfterMs);
    if (!_sendFC65(slaveId)) return false;
    Log(NOTIFY, "[BL] waiting %lums for device to reboot into BL...\r\n",
        (unsigned long)waitAfterMs);
    delay(waitAfterMs);
    Log(NOTIFY, "[BL] trigger wait done\r\n");
    return true;
}

bool ModBusBLMaster::connect(uint8_t slaveId) {
    uint8_t  magic[2] = { MBBP_MAGIC_H, MBBP_MAGIC_L };
    Log(NOTIFY, "[BL] HELLO → slave %u\r\n", (unsigned)slaveId);
    _sendFrame(slaveId, MBBP_CMD_HELLO, magic, 2);

    uint8_t  rsp[MBBP_MAX_PAYLOAD];
    uint16_t rlen = 0;
    uint8_t  cmd = _recvFrame(slaveId, rsp, &rlen, 2000);
    Log(NOTIFY, "[BL] HELLO response: cmd=0x%02X len=%u err=%s\r\n",
        cmd, (unsigned)rlen, _lastError);
    if (cmd != MBBP_RSP_HELLO) {
        _setError("HELLO failed");
        return false;
    }
    Log(NOTIFY, "[BL] HELLO OK — bootloader confirmed\r\n");
    return true;
}

bool ModBusBLMaster::flashFirmware(uint8_t slaveId,
                                    const uint8_t *firmware, uint32_t firmwareSize)
{
    uint16_t fw_crc = _crc16(firmware, (uint16_t)firmwareSize);

    /* START */
    uint8_t start_pay[6];
    start_pay[0] = (uint8_t)(firmwareSize >> 24);
    start_pay[1] = (uint8_t)(firmwareSize >> 16);
    start_pay[2] = (uint8_t)(firmwareSize >>  8);
    start_pay[3] = (uint8_t)(firmwareSize & 0xFF);
    start_pay[4] = (uint8_t)(fw_crc >> 8);
    start_pay[5] = (uint8_t)(fw_crc & 0xFF);
    _sendFrame(slaveId, MBBP_CMD_START, start_pay, 6);

    uint8_t  rsp[MBBP_MAX_PAYLOAD];
    uint16_t rlen = 0;
    if (_recvFrame(slaveId, rsp, &rlen, 2000) != MBBP_RSP_START) {
        _setError("START rejected");
        return false;
    }

    /* WRITE_PAGE */
    uint32_t total_pages = (firmwareSize + MBBP_PAGE_SIZE - 1) / MBBP_PAGE_SIZE;
    uint8_t  page_buf[2 + MBBP_PAGE_SIZE];

    for (uint32_t pg = 0; pg < total_pages; pg++) {
        page_buf[0] = (uint8_t)(pg >> 8);
        page_buf[1] = (uint8_t)(pg & 0xFF);

        uint32_t offset = pg * MBBP_PAGE_SIZE;
        for (uint8_t i = 0; i < MBBP_PAGE_SIZE; i++) {
            uint32_t src = offset + i;
            page_buf[2 + i] = (src < firmwareSize) ? firmware[src] : 0xFF;
        }

        _sendFrame(slaveId, MBBP_CMD_WRITE_PAGE, page_buf, 2 + MBBP_PAGE_SIZE);
        if (_recvFrame(slaveId, rsp, &rlen, 3000) != MBBP_RSP_WRITE_PAGE) {
            _setError("WRITE_PAGE rejected");
            return false;
        }

        if (_progressCb)
            _progressCb((uint8_t)((pg + 1) * 90 / total_pages));
    }

    /* VERIFY */
    _sendFrame(slaveId, MBBP_CMD_VERIFY, NULL, 0);
    if (_recvFrame(slaveId, rsp, &rlen, 5000) != MBBP_RSP_VERIFY_OK) {
        _setError("VERIFY failed");
        return false;
    }
    if (_progressCb) _progressCb(95);

    /* JUMP */
    _sendFrame(slaveId, MBBP_CMD_JUMP, NULL, 0);
    _recvFrame(slaveId, rsp, &rlen, 1000);

    if (_progressCb) _progressCb(100);
    return true;
}

bool ModBusBLMaster::updateFirmware(uint8_t slaveId, const uint8_t *firmware,
                                     uint32_t firmwareSize, uint8_t connectRetries,
                                     bool skipTrigger)
{
    Log(NOTIFY, "[BL] updateFirmware slave=%u fwSize=%lu retries=%u skipTrigger=%s\r\n",
        (unsigned)slaveId, (unsigned long)firmwareSize, (unsigned)connectRetries,
        skipTrigger ? "YES" : "no");

    if (!skipTrigger) {
        if (!triggerBootloader(slaveId)) {
            Log(ERROR, "[BL] triggerBootloader FAILED: %s\r\n", _lastError);
            return false;
        }
    } else {
        Log(NOTIFY, "[BL] skipping FC65 trigger — assuming device already in BL mode\r\n");
    }

    for (uint8_t i = 0; i < connectRetries; i++) {
        Log(NOTIFY, "[BL] connect attempt %u/%u\r\n", (unsigned)(i + 1), (unsigned)connectRetries);
        if (connect(slaveId)) goto connected;
        Log(ERROR, "[BL] connect attempt %u failed: %s — retrying in 200ms\r\n",
            (unsigned)(i + 1), _lastError);
        delay(200);
    }
    Log(ERROR, "[BL] connect FAILED after %u attempts\r\n", (unsigned)connectRetries);
    _setError("connect failed after retries");
    return false;

connected:
    Log(NOTIFY, "[BL] connected — starting flashFirmware\r\n");
    bool ok = flashFirmware(slaveId, firmware, firmwareSize);
    Log(ok ? NOTIFY : ERROR, "[BL] flashFirmware %s: %s\r\n", ok ? "OK" : "FAILED", _lastError);
    return ok;
}
