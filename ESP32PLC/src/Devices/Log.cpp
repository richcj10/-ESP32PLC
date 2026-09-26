#include "Log.h"
#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>
#include <esp_heap_caps.h>
#include "Webportal.h"

// ── PSRAM ring buffer ─────────────────────────────────────────────────────────
#define LOG_RING_LINES    64
#define LOG_RING_LINE_LEN 128   // 64 × 128 = 8 KB in PSRAM

static char    (*_logRing)[LOG_RING_LINE_LEN] = nullptr;
static uint8_t   _logHead  = 0;
static uint8_t   _logCount = 0;

void LogRingInit() {
    _logRing = (char(*)[LOG_RING_LINE_LEN])
               heap_caps_malloc(LOG_RING_LINES * LOG_RING_LINE_LEN, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_logRing)
        _logRing = (char(*)[LOG_RING_LINE_LEN]) malloc(LOG_RING_LINES * LOG_RING_LINE_LEN);
}

static void _ringPush(const char* line) {
    if (!_logRing) return;
    strlcpy(_logRing[_logHead], line, LOG_RING_LINE_LEN);
    _logHead = (_logHead + 1) % LOG_RING_LINES;
    if (_logCount < LOG_RING_LINES) _logCount++;
}

uint8_t LogRingCount() { return _logCount; }

const char* LogRingGet(uint8_t idx) {
    if (!_logRing || idx >= _logCount) return "";
    uint8_t start = (uint8_t)((_logHead - _logCount + LOG_RING_LINES) % LOG_RING_LINES);
    return _logRing[(start + idx) % LOG_RING_LINES];
}

// ── Level filter ──────────────────────────────────────────────────────────────
// Global, changed at runtime from the web Log tab (persisted in NVS by the caller).
// Log() rejects a filtered line with one compare — no formatting, no I/O.
volatile uint8_t g_logLevel = LOG;
static bool      _webLog    = false;

void LogSetup(char DebugLevel, bool WebPage) {
    LogSetLevel((uint8_t)DebugLevel);
    _webLog = WebPage;
}

void LogSetLevel(uint8_t level) {
    if (level < ERROR) level = ERROR;
    if (level > DEBUG) level = DEBUG;
    g_logLevel = level;
}

uint8_t LogGetLevel() { return g_logLevel; }

static const char* _prefix(char level) {
    switch (level) {
        case ERROR:        return "ERR>";
        case LOG:          return "LOG>";
        case NOTIFY:       return "NOTIFY>";
        case DEBUG:        return "DEBUG>";
        case NOTIFY_FORCE: return "NOTIFY_F>";
        default:           return "ALT>";
    }
}

char Log(char level, const char* format, ...) {
    // Fast reject — must stay first. NOTIFY_FORCE always passes.
    if (level >= ERROR && level <= DEBUG && (uint8_t)level > g_logLevel) return 0;

    // Stack buffer: thread-safe (Log is called from web/scan tasks too) and no heap
    // for normal lines. Only unusually long lines fall back to PSRAM.
    char  stackBuf[160];
    char* buf = stackBuf;
    size_t cap = sizeof(stackBuf);

    const char* pre = _prefix(level);
    size_t preLen = strlen(pre);
    memcpy(buf, pre, preLen);

    va_list arg;
    va_start(arg, format);
    va_list copy;
    va_copy(copy, arg);
    int len = vsnprintf(buf + preLen, cap - preLen, format, copy);
    va_end(copy);
    if (len < 0) { va_end(arg); return 0; }

    if ((size_t)len >= cap - preLen - 2) {          // didn't fit (+ room for \r\n)
        size_t need = preLen + (size_t)len + 3;
        char* big = (char*)ps_malloc(need);
        if (!big) big = (char*)malloc(need);
        if (big) {
            memcpy(big, pre, preLen);
            vsnprintf(big + preLen, need - preLen, format, arg);
            buf = big; cap = need;
        } else {
            len = (int)(cap - preLen - 3);          // truncate instead
        }
    }
    va_end(arg);

    // Strip trailing CR/LF the caller included, then add exactly one
    size_t end = preLen + (size_t)len;
    while (end > preLen && (buf[end-1] == '\n' || buf[end-1] == '\r')) end--;
    buf[end] = '\0';

    const char* msg = buf + preLen;
    _ringPush(msg);
    if (_webLog) WebLogSend(msg);

    // One buffered write per line (Serial TX buffer set in SystemStart) —
    // replaces three blocking ets_printf calls.
    buf[end] = '\r'; buf[end+1] = '\n';
    Serial.write((const uint8_t*)buf, end + 2);

    if (buf != stackBuf) free(buf);
    return 0;
}
