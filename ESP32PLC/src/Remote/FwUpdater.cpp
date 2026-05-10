#include "FwUpdater.h"
#include "MasterController.h"
#include "Webportal.h"
#include "Devices/Log.h"
#include <ModBusBLMaster.h>

/* ── Global active-updater pointer used by the static progress callback ──── */
static FwUpdater *g_activeUpdater = nullptr;

/* ── Task parameter ─────────────────────────────────────────────────────── */
struct FwTaskParam {
    FwUpdater *self;
    uint8_t    slaveId;
};

/* ── Constructor / init ─────────────────────────────────────────────────── */

FwUpdater::FwUpdater(HardwareSerial &serial, int8_t dirPin)
    : _serial(serial), _dirPin(dirPin) {}

void FwUpdater::init() {
    if (!LittleFS.exists("/fw"))
        LittleFS.mkdir("/fw");
}

/* ── Path helper ────────────────────────────────────────────────────────── */

String FwUpdater::fwPath(uint8_t slaveId) {
    return "/fw/" + String(slaveId) + ".bin";
}

/* ── Filesystem helpers ─────────────────────────────────────────────────── */

bool FwUpdater::hasFirmware(uint8_t slaveId) {
    return LittleFS.exists(fwPath(slaveId));
}

uint32_t FwUpdater::firmwareSize(uint8_t slaveId) {
    File f = LittleFS.open(fwPath(slaveId), "r");
    if (!f) return 0;
    uint32_t sz = f.size();
    f.close();
    return sz;
}

/* ── Intel HEX parser ───────────────────────────────────────────────────── */

static uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0;
}

static uint8_t hexByte(const char *p) {
    return (hexNibble(p[0]) << 4) | hexNibble(p[1]);
}

bool FwUpdater::_hexToBin(const uint8_t *hexData, size_t hexLen,
                           uint8_t *out, uint32_t *outSize)
{
    memset(out, 0xFF, FW_MAX_BIN_SIZE);
    uint32_t    maxAddr = 0;
    const char *p       = (const char *)hexData;
    const char *end     = p + hexLen;

    while (p < end) {
        while (p < end && *p != ':') p++;
        if (p >= end) break;
        p++; /* skip ':' */

        if (end - p < 10) break;

        uint8_t  byteCount = hexByte(p); p += 2;
        uint16_t addrH     = (uint16_t)hexByte(p); p += 2;
        uint16_t addrL     = (uint16_t)hexByte(p); p += 2;
        uint16_t addr      = (addrH << 8) | addrL;
        uint8_t  recType   = hexByte(p); p += 2;

        if (recType == 0x01) break; /* EOF */

        if (recType == 0x00) {
            /* Data record */
            for (uint8_t i = 0; i < byteCount; i++) {
                if (end - p < 2) return false;
                uint16_t dest = addr + i;
                if (dest >= FW_MAX_BIN_SIZE) return false;
                out[dest] = hexByte(p); p += 2;
                if ((uint32_t)(dest + 1) > maxAddr) maxAddr = dest + 1;
            }
        } else if (recType == 0x04) {
            /* Extended linear address — upper 16 bits of 32-bit address.
             * ATmega328P flash is max 32KB; upper word must be 0x0000.
             * A non-zero value means this image targets a device with
             * >64KB address space and cannot be flashed here.          */
            if (byteCount == 2 && end - p >= 4) {
                uint16_t upper = ((uint16_t)hexByte(p) << 8) | hexByte(p + 2);
                if (upper != 0) { *outSize = 0; return false; }
            }
            p += byteCount * 2;
        } else {
            p += byteCount * 2; /* skip other record types (0x02, 0x03, 0x05) */
        }

        p += 2; /* skip checksum */
        while (p < end && (*p == '\r' || *p == '\n')) p++;
    }

    *outSize = maxAddr;
    return maxAddr > 0;
}

/* ── Store firmware ─────────────────────────────────────────────────────── */

bool FwUpdater::storeFirmware(uint8_t slaveId, const uint8_t *data,
                               size_t len, bool isHex)
{
    if (!LittleFS.exists("/fw")) LittleFS.mkdir("/fw");

    if (isHex) {
        uint8_t *binBuf = (uint8_t *)ps_malloc(FW_MAX_BIN_SIZE);
        if (!binBuf)  binBuf = (uint8_t *)malloc(FW_MAX_BIN_SIZE);
        if (!binBuf) {
            Log(ERROR, "FwUpdater: buffer alloc failed\r\n");
            return false;
        }
        uint32_t binSize = 0;
        bool ok = _hexToBin(data, len, binBuf, &binSize);
        if (!ok || binSize == 0) {
            free(binBuf);
            Log(ERROR, "FwUpdater: HEX parse failed\r\n");
            return false;
        }
        File f = LittleFS.open(fwPath(slaveId), "w");
        if (!f) { free(binBuf); return false; }
        f.write(binBuf, binSize);
        f.close();
        free(binBuf);
        Log(LOG, "FwUpdater: stored %lu bytes for slave %u (from HEX)\r\n", binSize, slaveId);
    } else {
        File f = LittleFS.open(fwPath(slaveId), "w");
        if (!f) return false;
        f.write(data, len);
        f.close();
        Log(LOG, "FwUpdater: stored %u bytes for slave %u (BIN)\r\n", len, slaveId);
    }
    return true;
}

/* ── Progress callback (called from update task via ModBusBLMaster) ──────── */

void FwUpdater::_progressCb(uint8_t pct) {
    if (!g_activeUpdater) return;
    g_activeUpdater->_status.progress = pct;
    WebFwProgressSend(pct, false, false, nullptr);
}

/* ── FreeRTOS update task ───────────────────────────────────────────────── */

void FwUpdater::_updateTask(void *vParam) {
    auto  *p       = (FwTaskParam *)vParam;
    FwUpdater *self = p->self;
    uint8_t slaveId = p->slaveId;
    delete p;

    /* Read firmware from LittleFS into PSRAM */
    String path = fwPath(slaveId);
    File f = LittleFS.open(path, "r");
    if (!f) {
        strncpy(self->_status.message, "FW file not found", 63);
        goto done_fail;
    }
    {
        uint32_t fwSize = f.size();
        uint8_t *fwBuf  = (uint8_t *)ps_malloc(fwSize);
        if (!fwBuf)  fwBuf = (uint8_t *)malloc(fwSize);
        if (!fwBuf) {
            f.close();
            strncpy(self->_status.message, "Buffer alloc failed", 63);
            goto done_fail;
        }
        f.read(fwBuf, fwSize);
        f.close();

        /* Pause normal Modbus polling so we have exclusive RS-485 access */
        SuspendRemotePolling();
        vTaskDelay(pdMS_TO_TICKS(600));

        /* Run the flash sequence */
        g_activeUpdater = self;
        ModBusBLMaster bl(self->_serial, self->_dirPin);
        bl.begin(38400);
        bl.setProgressCallback(FwUpdater::_progressCb);

        bool ok = bl.updateFirmware(slaveId, fwBuf, fwSize);

        g_activeUpdater = nullptr;
        free(fwBuf);
        ResumeRemotePolling();

        if (ok) {
            strncpy(self->_status.message, "Update complete", 63);
            self->_status.success  = true;
            self->_status.progress = 100;
            self->_status.done     = true;
            self->_status.running  = false;
            WebFwProgressSend(100, true, true, "Update complete");
        } else {
            strncpy(self->_status.message, bl.lastError(), 63);
            self->_status.success = false;
            self->_status.done    = true;
            self->_status.running = false;
            WebFwProgressSend(0, true, false, bl.lastError());
        }
        self->_taskHandle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

done_fail:
    self->_status.success  = false;
    self->_status.done     = true;
    self->_status.running  = false;
    WebFwProgressSend(0, true, false, self->_status.message);
    self->_taskHandle = nullptr;
    vTaskDelete(nullptr);
}

/* ── startUpdate ────────────────────────────────────────────────────────── */

bool FwUpdater::startUpdate(uint8_t slaveId) {
    if (_status.running) {
        Log(ERROR, "FwUpdater: update already in progress\r\n");
        return false;
    }
    if (!hasFirmware(slaveId)) {
        Log(ERROR, "FwUpdater: no firmware stored for slave %u\r\n", slaveId);
        return false;
    }

    _status.running  = true;
    _status.done     = false;
    _status.success  = false;
    _status.progress = 0;
    _status.slaveId  = slaveId;
    snprintf(_status.message, sizeof(_status.message), "Starting update for slave %u", slaveId);

    auto *param    = new FwTaskParam{ this, slaveId };
    BaseType_t rc  = xTaskCreate(_updateTask, "fwUpdate", 8192, param, 1, &_taskHandle);
    if (rc != pdPASS) {
        delete param;
        _status.running = false;
        Log(ERROR, "FwUpdater: xTaskCreate failed\r\n");
        return false;
    }
    Log(LOG, "FwUpdater: update task started for slave %u\r\n", slaveId);
    return true;
}

/* ── Global instance ────────────────────────────────────────────────────── */
FwUpdater fwUpdater(Serial1, 20);
