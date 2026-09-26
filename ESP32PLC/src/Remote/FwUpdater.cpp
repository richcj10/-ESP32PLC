#include "FwUpdater.h"
#include "MasterController.h"
#include "Webportal.h"
#include "Devices/Log.h"
#include <ModBusBLMaster.h>
#include "FileSystem/FSInterface.h"

/* ── Global active-updater pointer used by the static progress callback ──── */
static FwUpdater *g_activeUpdater = nullptr;

/* ── Task parameter ─────────────────────────────────────────────────────── */
struct FwTaskParam {
    FwUpdater *self;
    uint8_t    slaveId;
    bool       skipTrigger;
};

/* ── Constructor / init ─────────────────────────────────────────────────── */

FwUpdater::FwUpdater(HardwareSerial &serial, int8_t dirPin)
    : _serial(serial), _dirPin(dirPin) {}

void FwUpdater::init() {
    if (!LittleFS.exists("/fw"))
        LittleFS.mkdir("/fw");
}

/* ── Path helper ────────────────────────────────────────────────────────── */

String FwUpdater::fwPath(uint8_t slaveId, uint8_t slot) {
    return "/fw/" + String(slaveId) + (slot == FW_SLOT_BL_UPDATER ? "_blupd.bin" : ".bin");
}

/* ── Filesystem helpers ─────────────────────────────────────────────────── */

bool FwUpdater::hasFirmware(uint8_t slaveId, uint8_t slot) {
    return LittleFS.exists(fwPath(slaveId, slot));
}

uint32_t FwUpdater::firmwareSize(uint8_t slaveId, uint8_t slot) {
    File f = LittleFS.open(fwPath(slaveId, slot), "r");
    if (!f) return 0;
    uint32_t sz = f.size();
    f.close();
    return sz;
}

/* ── Bootloader version record ('M' 'B' 'B' 'L' <ver> <~ver>) ─────────────── */

uint8_t FwUpdater::findBlVersion(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i + 6 <= len; i++) {
        if (buf[i] == 'M' && buf[i + 1] == 'B' && buf[i + 2] == 'B' && buf[i + 3] == 'L' &&
            (uint8_t)(buf[i + 4] ^ buf[i + 5]) == 0xFF)
            return buf[i + 4];
    }
    return 0;
}

uint8_t FwUpdater::updaterTargetVersion(uint8_t slaveId) {
    File f = LittleFS.open(fwPath(slaveId, FW_SLOT_BL_UPDATER), "r");
    if (!f) return 0;
    uint32_t sz = f.size();
    uint8_t *buf = (uint8_t *)ps_malloc(sz);
    if (!buf) buf = (uint8_t *)malloc(sz);
    uint8_t v = 0;
    if (buf) {
        if (f.read(buf, sz) == sz) v = findBlVersion(buf, sz);
        free(buf);
    }
    f.close();
    return v;
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
                               size_t len, bool isHex, uint8_t slot)
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
        File f = LittleFS.open(fwPath(slaveId, slot), "w");
        if (!f) { free(binBuf); return false; }
        f.write(binBuf, binSize);
        f.close();
        free(binBuf);
        Log(LOG, "FwUpdater: stored %lu bytes for slave %u (from HEX)\r\n", binSize, slaveId);
    } else {
        File f = LittleFS.open(fwPath(slaveId, slot), "w");
        if (!f) return false;
        f.write(data, len);
        f.close();
        Log(LOG, "FwUpdater: stored %u bytes for slave %u (BIN)\r\n", len, slaveId);
    }
    return true;
}

/* ── Progress callback (called from update task via ModBusBLMaster) ──────── */

// A guided bootloader update runs two flashes; each maps its 0-100 % onto part
// of the overall bar (g_progBase .. g_progBase + g_progSpan).
static uint8_t g_progBase = 0;
static uint8_t g_progSpan = 100;

// Called once per flashed page (~100 per app). Only push to the browser when the
// percentage changes, at most every 200 ms: a message per page overflowed the
// WebSocket queue (32), and the queue then dropped the later step/done messages.
void FwUpdater::_progressCb(uint8_t pct) {
    if (!g_activeUpdater) return;
    uint8_t overall = (uint8_t)(g_progBase + (uint16_t)pct * g_progSpan / 100);
    g_activeUpdater->_status.progress = overall;
    static uint8_t  lastSent   = 0xFF;
    static uint32_t lastSentMs = 0;
    if (overall == lastSent || millis() - lastSentMs < 200) return;
    lastSent = overall; lastSentMs = millis();
    WebFwProgressSend(overall, false, false,
                      g_activeUpdater->_status.blUpdate ? g_activeUpdater->_status.message : nullptr);
}

/* ── FreeRTOS update task ───────────────────────────────────────────────── */

void FwUpdater::_updateTask(void *vParam) {
    auto  *p          = (FwTaskParam *)vParam;
    FwUpdater *self   = p->self;
    uint8_t slaveId   = p->slaveId;
    bool skipTrigger  = p->skipTrigger;
    delete p;

    /* Read firmware from LittleFS into PSRAM */
    String path = fwPath(slaveId);
    Log(LOG, "FwTask: opening %s\r\n", path.c_str());
    File f = LittleFS.open(path, "r");
    if (!f) {
        Log(ERROR, "FwTask: file not found — %s\r\n", path.c_str());
        strncpy(self->_status.message, "FW file not found", 63);
        goto done_fail;
    }
    {
        uint32_t fwSize = f.size();
        Log(LOG, "FwTask: firmware size=%lu B — allocating buffer\r\n", (unsigned long)fwSize);
        uint8_t *fwBuf  = (uint8_t *)ps_malloc(fwSize);
        if (!fwBuf)  fwBuf = (uint8_t *)malloc(fwSize);
        if (!fwBuf) {
            f.close();
            Log(ERROR, "FwTask: buffer alloc failed (heap=%u PSRAM=%u)\r\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
            strncpy(self->_status.message, "Buffer alloc failed", 63);
            goto done_fail;
        }
        size_t rd = f.read(fwBuf, fwSize);
        f.close();
        Log(LOG, "FwTask: read %u of %lu bytes from LittleFS\r\n", (unsigned)rd, (unsigned long)fwSize);

        /* Pause normal Modbus polling so we have exclusive RS-485 access */
        Log(LOG, "FwTask: suspending Modbus polling\r\n");
        SuspendRemotePolling();
        vTaskDelay(pdMS_TO_TICKS(600));

        /* Run the flash sequence */
        Log(LOG, "FwTask: starting bl.updateFirmware for slave %u\r\n", (unsigned)slaveId);
        g_activeUpdater = self;
        g_progBase = 0; g_progSpan = 100;
        ModBusBLMaster bl(self->_serial, self->_dirPin);
        bl.begin(38400);
        bl.setProgressCallback(FwUpdater::_progressCb);

        bool ok = bl.updateFirmware(slaveId, fwBuf, fwSize, 5, skipTrigger);

        g_activeUpdater = nullptr;
        free(fwBuf);
        Log(LOG, "FwTask: resuming Modbus polling\r\n");
        ResumeRemotePolling();

        if (ok) {
            Log(LOG, "FwTask: update complete for slave %u\r\n", (unsigned)slaveId);
            strncpy(self->_status.message, "Update complete", 63);
            self->_status.success  = true;
            self->_status.progress = 100;
            self->_status.done     = true;
            self->_status.running  = false;
            WebFwProgressSend(100, true, true, "Update complete");
        } else {
            Log(ERROR, "FwTask: update FAILED for slave %u — %s\r\n",
                (unsigned)slaveId, bl.lastError());
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
    Log(ERROR, "FwTask: done_fail — %s\r\n", self->_status.message);
    self->_status.success  = false;
    self->_status.done     = true;
    self->_status.running  = false;
    WebFwProgressSend(0, true, false, self->_status.message);
    self->_taskHandle = nullptr;
    vTaskDelete(nullptr);
}

/* ── startUpdate ────────────────────────────────────────────────────────── */

bool FwUpdater::startUpdate(uint8_t slaveId, bool skipTrigger) {
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
    _status.blUpdate = false;
    snprintf(_status.message, sizeof(_status.message),
             "Starting update for slave %u%s", slaveId, skipTrigger ? " (skip trigger)" : "");

    auto *param    = new FwTaskParam{ this, slaveId, skipTrigger };
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

/* ── Guided bootloader update ───────────────────────────────────────────── */

#define BL_FALLBACK_ADDR     200      // new bootloader's MBBP_SLAVE_ADDR when the EEPROM record is invalid
#define BL_ADDR_CHANGE_KEY   0xA5B0   // FC16 reg1 key for "reg0 = new slave ID" (ModBusBL app)
#define BL_REBOOT_WAIT_MS    3000     // updater rewrites 2 KB of flash, then resets
#define BL_FIND_TIMEOUT_MS   30000    // how long to look for the new bootloader
#define BL_APP_BOOT_MS       1500     // app start-up after JUMP
#define BL_VERIFY_TIMEOUT_MS 6000     // how long to wait for the app to answer

static bool _loadImage(uint8_t id, uint8_t slot, uint8_t **buf, uint32_t *size) {
    File f = LittleFS.open(FwUpdater::fwPath(id, slot), "r");
    if (!f) return false;
    *size = f.size();
    *buf  = (uint8_t *)ps_malloc(*size);
    if (!*buf) *buf = (uint8_t *)malloc(*size);
    if (!*buf) { f.close(); return false; }
    size_t rd = f.read(*buf, *size);
    f.close();
    return rd == *size && *size > 0;
}

// Address 200 is only probed when nothing lives there: MBBP HELLO uses the
// same function byte (0x41) as the FC65 "enter bootloader" command, so a HELLO
// to a running app at 200 would reboot that app into its bootloader.
// Two checks, both needed: nothing configured at 200, and no running app
// answers FC03 there (catches devices that aren't in Remote.json).
// Call with polling suspended — the probe uses the bus directly.
static bool _fallbackAddrFree(uint8_t id) {
    if (id == BL_FALLBACK_ADDR) return false;
    const RemoteConfig_t &cfg = GetRemoteConfig();
    for (uint8_t i = 0; i < cfg.deviceCount; i++)
        if (cfg.devices[i].address == BL_FALLBACK_ADDR) {
            Log(NOTIFY_FORCE, "BLUpdate: %u is in Remote.json - won't probe it", BL_FALLBACK_ADDR);
            return false;
        }
    if (ProbeModbusAddr(BL_FALLBACK_ADDR)) {
        Log(NOTIFY_FORCE, "BLUpdate: a running device answers at %u - won't probe it", BL_FALLBACK_ADDR);
        return false;
    }
    return true;
}

void FwUpdater::_finish(bool ok, const char *msg) {
    g_activeUpdater = nullptr;
    g_progBase = 0; g_progSpan = 100;
    strlcpy(_status.message, msg, sizeof(_status.message));
    _status.success  = ok;
    if (ok) _status.progress = 100;
    _status.done     = true;
    _status.running  = false;
    _status.blUpdate = false;
    Log(ok ? NOTIFY_FORCE : ERROR, "BLUpdate[%u]: %s", (unsigned)_status.slaveId, msg);
    WebFwProgressSend(ok ? 100 : _status.progress, true, ok, msg);
}

void FwUpdater::_blUpdateTask(void *vParam) {
    auto      *p    = (FwTaskParam *)vParam;
    FwUpdater *self = p->self;
    uint8_t    id   = p->slaveId;
    delete p;

    char     msg[64];
    uint8_t *upd = nullptr, *app = nullptr;
    uint32_t updSz = 0, appSz = 0;
    uint8_t  found = 0;
    uint8_t  oldVer = 0, newVer = 0;   // bootloader versions before / after
    uint8_t  target = 0;               // version the updater image installs (0 = unknown)
    bool     ok = false;

    auto step = [&](uint8_t pct, const char *fmt, unsigned a = 0) {
        snprintf(self->_status.message, sizeof(self->_status.message), fmt, a);
        self->_status.progress = pct;
        Log(NOTIFY_FORCE, "BLUpdate[%u]: %s", (unsigned)id, self->_status.message);
        WebFwProgressSend(pct, false, false, self->_status.message);
    };

    if (!_loadImage(id, FW_SLOT_BL_UPDATER, &upd, &updSz)) {
        free(upd); self->_finish(false, "Updater image missing or unreadable"); goto out;
    }
    if (!_loadImage(id, FW_SLOT_APP, &app, &appSz)) {
        free(upd); free(app); self->_finish(false, "App image missing or unreadable"); goto out;
    }
    target = findBlVersion(upd, updSz);
    Log(NOTIFY_FORCE, "BLUpdate[%u]: updater installs bootloader v%u", (unsigned)id, (unsigned)target);

    SuspendRemotePolling();
    vTaskDelay(pdMS_TO_TICKS(600));
    {
        const bool try200 = _fallbackAddrFree(id);   // bus is ours now; checked before anything moves
        g_activeUpdater = self;
        ModBusBLMaster bl(self->_serial, self->_dirPin);
        bl.begin(38400);
        bl.setProgressCallback(FwUpdater::_progressCb);

        // 1 ── flash the updater like a normal app (0-40 %)
        step(0, "1/5 Flashing bootloader updater to %u", id);
        g_progBase = 0; g_progSpan = 40;
        if (!bl.updateFirmware(id, upd, updSz, 5, false)) {
            // The first WRITE_PAGE clears APP_VALID, so the app may no longer start —
            // but the bootloader itself is untouched: re-Flash the app to recover.
            snprintf(msg, sizeof(msg), "Updater flash failed: %s", bl.lastError());
            Log(ERROR, "BLUpdate[%u]: bootloader untouched; app may be invalid - re-Flash it (skip trigger)",
                (unsigned)id);
            self->_finish(false, msg);
            goto resume;
        }
        oldVer = bl.blVersion();          // HELLO above was answered by the old bootloader

        // 2 ── updater rewrites the bootloader and resets; find the new bootloader
        step(42, "2/5 Updater running - waiting for new bootloader");
        vTaskDelay(pdMS_TO_TICKS(BL_REBOOT_WAIT_MS));
        {
            unsigned long t0 = millis();
            while (!found && millis() - t0 < BL_FIND_TIMEOUT_MS) {
                if (bl.connect(id))                          found = id;
                else if (try200 && bl.connect(BL_FALLBACK_ADDR)) found = BL_FALLBACK_ADDR;
            }
        }
        if (!found) {
            self->_finish(false, try200 ? "No bootloader answered. LED 7 blinks = needs ISP"
                                        : "No bootloader at own addr; 200 in use so not probed");
            goto resume;
        }
        newVer = bl.blVersion();          // answered by whatever bootloader is there now

        // 3 ── flash the app into the new bootloader (50-90 %)
        if (found == id) step(48, "3/5 Bootloader found at %u - flashing app", id);
        else             step(48, "3/5 Bootloader reset to 200 - flashing app");
        g_progBase = 50; g_progSpan = 40;
        if (!bl.updateFirmware(found, app, appSz, 5, true)) {
            snprintf(msg, sizeof(msg), "App flash failed at %u (%s) - retry Flash", found, bl.lastError());
            self->_finish(false, msg);
            goto resume;
        }
        g_activeUpdater = nullptr;
        vTaskDelay(pdMS_TO_TICKS(BL_APP_BOOT_MS));

        // 4 ── restore the original address if the bootloader fell back to 200
        if (found != id) {
            step(92, "4/5 Moving device from 200 back to %u", id);
            uint16_t vals[2] = { id, BL_ADDR_CHANGE_KEY };
            bool moved = false;
            for (uint8_t i = 0; i < 3 && !moved; i++) {
                moved = RawWriteRegs(BL_FALLBACK_ADDR, 0, vals, 2);
                if (!moved) vTaskDelay(pdMS_TO_TICKS(500));
            }
            if (!moved) {
                snprintf(msg, sizeof(msg), "App runs at 200; address change failed - use Addr to set %u", id);
                self->_finish(false, msg);
                goto resume;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            step(92, "4/5 Address %u kept", id);
        }

        // 5 ── confirm the app answers at its original address
        step(95, "5/5 Checking app answers at %u", id);
        {
            unsigned long t0 = millis();
            while (!ok && millis() - t0 < BL_VERIFY_TIMEOUT_MS) {
                ok = ProbeModbusAddr(id);
                if (!ok) vTaskDelay(pdMS_TO_TICKS(300));
            }
        }
        if (!ok) {
            snprintf(msg, sizeof(msg), "App flashed but no reply at %u - check device", id);
        } else if (target && oldVer == target && newVer == target) {
            snprintf(msg, sizeof(msg), "Bootloader already v%u - nothing to do, app at %u", newVer, id);
        } else if (newVer > oldVer && (!target || newVer == target)) {
            snprintf(msg, sizeof(msg), "Bootloader v%u -> v%u, app running at %u", oldVer, newVer, id);
        } else if (target && target <= oldVer) {
            // Updater refuses same/older versions (4 blinks) — nothing was written
            snprintf(msg, sizeof(msg), "Refused: image v%u not newer than v%u - app at %u", target, oldVer, id);
            ok = false;
        } else {
            // App is back, but the old bootloader still answers: the updater
            // refused (5 blinks = unsafe) or the write did not take
            snprintf(msg, sizeof(msg), "Bootloader still v%u (not replaced) - app at %u", newVer, id);
            ok = false;
        }
        self->_finish(ok, msg);
    }
resume:
    free(upd);
    free(app);
    ResumeRemotePolling();
out:
    self->_taskHandle = nullptr;
    vTaskDelete(nullptr);
}

bool FwUpdater::startBootloaderUpdate(uint8_t slaveId) {
    if (_status.running) {
        Log(ERROR, "FwUpdater: update already in progress\r\n");
        return false;
    }
    if (!hasFirmware(slaveId, FW_SLOT_BL_UPDATER) || !hasFirmware(slaveId, FW_SLOT_APP)) {
        Log(ERROR, "FwUpdater: bootloader update for %u needs updater + app images\r\n", slaveId);
        return false;
    }
    _status.running  = true;
    _status.done     = false;
    _status.success  = false;
    _status.progress = 0;
    _status.slaveId  = slaveId;
    _status.blUpdate = true;
    snprintf(_status.message, sizeof(_status.message), "Bootloader update for %u starting", slaveId);

    auto *param   = new FwTaskParam{ this, slaveId, false };
    BaseType_t rc = xTaskCreate(_blUpdateTask, "blUpdate", 8192, param, 1, &_taskHandle);
    if (rc != pdPASS) {
        delete param;
        _status.running  = false;
        _status.blUpdate = false;
        Log(ERROR, "FwUpdater: xTaskCreate failed\r\n");
        return false;
    }
    return true;
}

/* ── Global instance ────────────────────────────────────────────────────── */
FwUpdater fwUpdater(Serial1, 20);
