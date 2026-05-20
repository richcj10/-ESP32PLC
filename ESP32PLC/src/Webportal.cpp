#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "Sensors.h"
#include "Define.h"
#include "Functions.h"
#include "Remote/FwUpdater.h"
#include "Remote/MasterController.h"

#include "Devices/Log.h"
#include <esp_heap_caps.h>

#define HTTP_PORT 80

void WSRunJSON();

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

StaticJsonDocument<200> jsonDocTx;
StaticJsonDocument<300> jsonDocRx;

bool wsconnected = false;
bool lastButtonState = 0;
static char* output = nullptr;

static constexpr size_t OUTPUT_BUF_SIZE = 512;

unsigned long cnt = 0;
unsigned long LastTime = 0;

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    wsconnected = true;
    Log(NOTIFY,"ws[%s][%u] connect\r\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    wsconnected = false;
    Log(NOTIFY,"ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    Log(ERROR,"WS Error");
  } else if (type == WS_EVT_PONG) {
    Log(NOTIFY,"WS pong");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      // the whole message is in a single frame and we got all of it's data
      Log(NOTIFY,"ws[%s][%u] %s-msg[%llu]\r\n", server->url(), client->id(),
          (info->opcode == WS_TEXT) ? "txt" : "bin", info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char)data[i];
        }
        Log(NOTIFY,"%s\r\n\r\n", msg.c_str());

        deserializeJson(jsonDocRx, msg);
        WSRunJSON();
        jsonDocRx.clear();
      }
    }
  }
}

/* ── WebSocket FW progress event ────────────────────────────────────────── */

void WebFwProgressSend(uint8_t percent, bool done, bool success, const char *msg) {
    char buf[160];
    int n = snprintf(buf, sizeof(buf),
        "{\"Type\":20,\"FW_PROG\":%u,\"FW_DONE\":%s,\"FW_OK\":%s",
        percent, done ? "true" : "false", success ? "true" : "false");
    if (msg && msg[0])
        snprintf(buf + n, sizeof(buf) - n, ",\"FW_MSG\":\"%s\"}", msg);
    else
        strncat(buf, "}", sizeof(buf) - strlen(buf) - 1);
    ws.textAll(buf);
}

/* ── FW upload context (one upload at a time) ───────────────────────────── */

static struct {
    uint8_t *hexBuf  = nullptr;
    size_t   hexCap  = 0;
    size_t   hexLen  = 0;
    uint8_t  slaveId = 0;
    bool     isHex   = false;
    bool     ok      = false;
    bool     aborted = false; /* set on size violation or alloc failure */
    bool     writeOk = true;  /* cleared on any LittleFS write error    */
    char     msg[80] = {};
} fwUp;

static void _onFwUpload(AsyncWebServerRequest *req,
                        const String &filename, size_t index,
                        uint8_t *data, size_t len, bool final)
{
    if (index == 0) {
        String idStr  = req->hasParam("id") ? req->getParam("id")->value() : "0";
        fwUp.slaveId  = (uint8_t)idStr.toInt();
        fwUp.isHex    = filename.endsWith(".hex") || (len > 0 && data[0] == ':');
        fwUp.ok       = false;
        fwUp.aborted  = false;
        fwUp.writeOk  = true;
        fwUp.hexLen   = 0;
        fwUp.msg[0]   = '\0';

        /* Size guard for raw binary — hex files are parsed after receipt */
        if (!fwUp.isHex && req->contentLength() > FW_MAX_BIN_SIZE) {
            fwUp.aborted = true;
            snprintf(fwUp.msg, sizeof(fwUp.msg),
                     "File too large (%u B, max %u B)",
                     (unsigned)req->contentLength(), FW_MAX_BIN_SIZE);
            return;
        }

        if (fwUp.isHex) {
            if (!fwUp.hexBuf) {
                fwUp.hexBuf = (uint8_t *)ps_malloc(FW_MAX_HEX_SIZE);
                if (!fwUp.hexBuf) fwUp.hexBuf = (uint8_t *)malloc(FW_MAX_HEX_SIZE);
                fwUp.hexCap = FW_MAX_HEX_SIZE;
            }
            if (!fwUp.hexBuf) {
                fwUp.aborted = true;
                strncpy(fwUp.msg, "Buffer alloc failed", sizeof(fwUp.msg) - 1);
                return;
            }
        } else {
            fwUpdater.init();
            if (fwUp.hexBuf) { free(fwUp.hexBuf); fwUp.hexBuf = nullptr; }
        }
    }

    if (fwUp.aborted) return;

    if (fwUp.isHex) {
        if (fwUp.hexBuf && fwUp.hexLen + len <= fwUp.hexCap) {
            memcpy(fwUp.hexBuf + fwUp.hexLen, data, len);
            fwUp.hexLen += len;
        }
    } else {
        File f = LittleFS.open(FwUpdater::fwPath(fwUp.slaveId),
                               index == 0 ? "w" : "a");
        if (!f) {
            fwUp.writeOk = false;
        } else {
            if (f.write(data, len) != len) fwUp.writeOk = false;
            f.close();
        }
    }

    if (final) {
        if (fwUp.aborted) {
            /* msg already set in index==0 */
        } else if (fwUp.isHex) {
            if (fwUp.hexBuf && fwUp.hexLen > 0) {
                fwUpdater.init();
                fwUp.ok = fwUpdater.storeFirmware(fwUp.slaveId, fwUp.hexBuf,
                                                   fwUp.hexLen, true);
                snprintf(fwUp.msg, sizeof(fwUp.msg),
                         fwUp.ok ? "HEX converted and stored" : "HEX parse failed");
            } else {
                strncpy(fwUp.msg, "No HEX data received", sizeof(fwUp.msg) - 1);
            }
        } else {
            fwUp.ok = fwUp.writeOk && fwUpdater.hasFirmware(fwUp.slaveId);
            if (fwUp.ok)
                snprintf(fwUp.msg, sizeof(fwUp.msg), "BIN stored (%lu B)",
                         (unsigned long)fwUpdater.firmwareSize(fwUp.slaveId));
            else if (!fwUp.writeOk)
                strncpy(fwUp.msg, "Write error — LittleFS full?", sizeof(fwUp.msg) - 1);
            else
                strncpy(fwUp.msg, "Write failed", sizeof(fwUp.msg) - 1);
        }
    }
}

/* ── Remote.json upload context ─────────────────────────────────────────────── */

static struct {
    bool ok      = false;
    bool aborted = false;
    char msg[80] = {};
} remUp;

static void _onRemoteUpload(AsyncWebServerRequest *req,
                            const String& /*filename*/, size_t index,
                            uint8_t *data, size_t len, bool final)
{
    if (index == 0) {
        remUp.ok      = false;
        remUp.aborted = false;
        remUp.msg[0]  = '\0';
        if (req->contentLength() > 16384) {
            remUp.aborted = true;
            strncpy(remUp.msg, "File too large (max 16 KB)", sizeof(remUp.msg) - 1);
            return;
        }
    }
    if (remUp.aborted) return;

    File f = LittleFS.open("/Remote.json", index == 0 ? "w" : "a");
    if (!f) {
        remUp.aborted = true;
        strncpy(remUp.msg, "FS open failed", sizeof(remUp.msg) - 1);
        return;
    }
    if (f.write(data, len) != len) {
        remUp.aborted = true;
        strncpy(remUp.msg, "Write error — LittleFS full?", sizeof(remUp.msg) - 1);
    }
    f.close();

    if (final && !remUp.aborted) {
        remUp.ok = true;
        strncpy(remUp.msg, "Remote.json saved — reboot to apply", sizeof(remUp.msg) - 1);
        Log(NOTIFY, "Web: Remote.json uploaded (%u B)\r\n", (unsigned)(index + len));
    }
}

/* ── Config body accumulation buffer (one request at a time) ───────────────── */
static constexpr size_t CFG_BUF_SIZE = 512;
static char*  _cfgBuf    = nullptr;
static size_t _cfgBufLen = 0;

static void _cfgBodyHandler(AsyncWebServerRequest*, uint8_t *data, size_t len,
                             size_t index, size_t total) {
    if (!_cfgBuf) return;
    if (index == 0) _cfgBufLen = 0;
    if (_cfgBufLen + len < CFG_BUF_SIZE - 1) {
        memcpy(_cfgBuf + _cfgBufLen, data, len);
        _cfgBufLen += len;
    }
    if (index + len >= total) _cfgBuf[_cfgBufLen] = '\0';
}

/* ── Device config body buffer (larger — holds full Remote.json) ────────── */
static constexpr size_t DEV_BUF_SIZE = 8192;
static char*  _devBuf    = nullptr;
static size_t _devBufLen = 0;

static void _devBodyHandler(AsyncWebServerRequest*, uint8_t *data, size_t len,
                            size_t index, size_t total) {
    if (!_devBuf) return;
    if (index == 0) _devBufLen = 0;
    if (_devBufLen + len < DEV_BUF_SIZE - 1) {
        memcpy(_devBuf + _devBufLen, data, len);
        _devBufLen += len;
    }
    if (index + len >= total) _devBuf[_devBufLen] = '\0';
}

/* ── Captive portal ─────────────────────────────────────────────────────── */
static DNSServer _dns;
static bool      _captive = false;

void CaptivePortalLoop() {
    if (_captive) _dns.processNextRequest();
}

static void _captiveRedirect(AsyncWebServerRequest *req) {
    req->redirect("http://192.168.4.1/");
}

static void _scheduleRestart() {
    TimerHandle_t t = xTimerCreate("rst", pdMS_TO_TICKS(800), pdFALSE, nullptr,
        [](TimerHandle_t h) { xTimerDelete(h, 0); ESP.restart(); });
    if (t) xTimerStart(t, 0);
}

void WebStart(){
  // Allocate shared response buffers in PSRAM
  output   = (char*) heap_caps_malloc(OUTPUT_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!output)   output   = (char*) malloc(OUTPUT_BUF_SIZE);
  _cfgBuf  = (char*) heap_caps_malloc(CFG_BUF_SIZE,    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!_cfgBuf)  _cfgBuf  = (char*) malloc(CFG_BUF_SIZE);
  _devBuf  = (char*) heap_caps_malloc(DEV_BUF_SIZE,    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!_devBuf)  _devBuf  = (char*) malloc(DEV_BUF_SIZE);

  Log(DEBUG,"Web Server Start!");

  /* ── Captive portal (AP mode only) ───────────────────────────────────── */
  if (GetWiFiMode() == WIFI_AP_MODE) {
    _captive = true;
    _dns.start(53, "*", WiFi.softAPIP());
    Log(LOG, "Web: captive portal active — DNS wildcard → %s\r\n",
        WiFi.softAPIP().toString().c_str());

    // OS-specific captive-portal detection URLs — must be before serveStatic
    server.on("/generate_204",               HTTP_GET, _captiveRedirect); // Android
    server.on("/gen_204",                    HTTP_GET, _captiveRedirect); // Android alt
    server.on("/hotspot-detect.html",        HTTP_GET, _captiveRedirect); // iOS / macOS
    server.on("/library/test/success.html",  HTTP_GET, _captiveRedirect); // iOS alt
    server.on("/ncsi.txt",                   HTTP_GET, _captiveRedirect); // Windows
    server.on("/connecttest.txt",            HTTP_GET, _captiveRedirect); // Windows 10
    server.on("/redirect",                   HTTP_GET, _captiveRedirect); // generic
    server.on("/canonical.html",             HTTP_GET, _captiveRedirect); // macOS alt
  }

  /* POST /api/reboot */
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *req) {
    req->send(200, "application/json", "{\"ok\":true,\"msg\":\"Rebooting\"}");
    _scheduleRestart();
  });

  /* ── API routes first — must be before serveStatic ──────────────────── */

  /* ── Config: GET /config/wifi ──────────────────────────────────────────── */
  server.on("/config/wifi", HTTP_GET, [](AsyncWebServerRequest *req) {
    Preferences p; p.begin("wifi", true);
    bool nvs = p.isKey("mode"); p.end();
    char buf[280];
    snprintf(buf, sizeof(buf),
        "{\"mode\":%u,\"ssid\":\"%s\",\"host\":\"%s\",\"dhcp\":%u,\"ip\":\"%s\",\"nvs\":%s}",
        (unsigned)GetWiFiMode(), GetSSID().c_str(), GetHostName().c_str(),
        1, GetIPStr().c_str(), nvs ? "true" : "false");
    req->send(200, "application/json", buf);
  });

  /* ── Config: POST /config/wifi ─────────────────────────────────────────── */
  server.on("/config/wifi", HTTP_POST,
    [](AsyncWebServerRequest *req) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, _cfgBuf, _cfgBufLen)) {
            req->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid JSON\"}");
            return;
        }
        uint8_t    mode = doc["mode"] | (uint8_t)2;
        const char *ssid = doc["ssid"] | "";
        const char *pass = doc["pass"] | "";
        const char *host = doc["host"] | "ESP32PLC";
        if (mode == 1 && strlen(ssid) == 0) {
            req->send(400, "application/json", "{\"ok\":false,\"error\":\"STA mode requires SSID\"}");
            return;
        }
        SaveWiFiConfig(mode, ssid, pass, host);
        Log(LOG, "Config: WiFi updated via web (mode=%u ssid=%s) — restarting\r\n", mode, ssid);
        char resp[160];
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"msg\":\"Saved. Restarting...\",\"mode\":%u,\"ssid\":\"%s\",\"host\":\"%s\"}",
            mode, ssid, host);
        req->send(200, "application/json", resp);
        _scheduleRestart();
    },
    nullptr,
    _cfgBodyHandler
  );

  /* ── Config: GET /config/mqtt ──────────────────────────────────────────── */
  server.on("/config/mqtt", HTTP_GET, [](AsyncWebServerRequest *req) {
    char buf[200];
    snprintf(buf, sizeof(buf),
        "{\"enabled\":%s,\"ip\":\"%s\",\"port\":%u,\"user\":\"%s\"}",
        GetMQTTEnabled() ? "true" : "false",
        GetMQTTIP().c_str(), (unsigned)GetMQTTPort(), GetMQTTUser().c_str());
    req->send(200, "application/json", buf);
  });

  /* ── Config: POST /config/mqtt ─────────────────────────────────────────── */
  server.on("/config/mqtt", HTTP_POST,
    [](AsyncWebServerRequest *req) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, _cfgBuf, _cfgBufLen)) {
            req->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid JSON\"}");
            return;
        }
        bool       enabled = doc["enabled"] | false;
        const char *ip     = doc["ip"]   | "";
        uint16_t   port    = doc["port"] | (uint16_t)1883;
        const char *user   = doc["user"] | "esp32plc";
        const char *pass   = doc["pass"] | "";
        if (enabled && strlen(ip) == 0) {
            req->send(400, "application/json", "{\"ok\":false,\"error\":\"IP required when enabled\"}");
            return;
        }
        SaveMQTTConfig(enabled, ip, port, user, pass);
        Log(LOG, "Config: MQTT updated via web (enabled=%u ip=%s) — restarting\r\n", enabled, ip);
        char resp[160];
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"msg\":\"Saved. Restarting...\",\"enabled\":%s,\"ip\":\"%s\",\"port\":%u}",
            enabled ? "true" : "false", ip, port);
        req->send(200, "application/json", resp);
        _scheduleRestart();
    },
    nullptr,
    _cfgBodyHandler
  );

  /* ── Config: GET /config/backup — download full config as JSON file ──────── */
  server.on("/config/backup", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<512> doc;
    doc["device"] = GetHostName();
    JsonObject w  = doc.createNestedObject("wifi");
    w["mode"] = GetWiFiMode();
    w["ssid"] = GetSSID();
    w["pass"] = GetSSIDPassword();
    w["host"] = GetHostName();
    JsonObject m  = doc.createNestedObject("mqtt");
    m["enabled"] = GetMQTTEnabled();
    m["ip"]      = GetMQTTIP();
    m["port"]    = GetMQTTPort();
    m["user"]    = GetMQTTUser();
    m["pass"]    = GetMQTTPassword();
    char buf[512];
    serializeJson(doc, buf, sizeof(buf));
    AsyncWebServerResponse *resp = req->beginResponse(200, "application/json", buf);
    resp->addHeader("Content-Disposition",
                    "attachment; filename=\"esp32plc-config.json\"");
    req->send(resp);
  });

  /* ── Config: POST /config/restore — restore backup JSON + restart ─────── */
  server.on("/config/restore", HTTP_POST,
    [](AsyncWebServerRequest *req) {
      StaticJsonDocument<512> doc;
      if (deserializeJson(doc, _cfgBuf, _cfgBufLen)) {
        req->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid JSON\"}");
        return;
      }
      bool saved = false;
      if (doc.containsKey("wifi")) {
        JsonObject w = doc["wifi"];
        SaveWiFiConfig(w["mode"] | (uint8_t)2,
                       w["ssid"] | "", w["pass"] | "", w["host"] | "");
        saved = true;
      }
      if (doc.containsKey("mqtt")) {
        JsonObject m = doc["mqtt"];
        SaveMQTTConfig(m["enabled"] | false, m["ip"] | "",
                       m["port"] | (uint16_t)1883,
                       m["user"] | "esp32plc", m["pass"] | "");
        saved = true;
      }
      if (!saved) {
        req->send(400, "application/json",
                  "{\"ok\":false,\"error\":\"no wifi or mqtt key\"}");
        return;
      }
      req->send(200, "application/json",
                "{\"ok\":true,\"msg\":\"Restored. Restarting...\"}");
      _scheduleRestart();
    },
    nullptr,
    _cfgBodyHandler
  );

  /* GET /fw/list — JSON device + firmware status */
  server.on("/fw/list", HTTP_GET, [](AsyncWebServerRequest *req) {
    FwDeviceInfo devs[8];
    uint8_t count = GetFwDeviceList(devs, 8);
    const FwUpdater::Status &st = fwUpdater.getStatus();

    char buf[512];
    int n = snprintf(buf, sizeof(buf),
        "{\"updating\":%s,\"slaveId\":%u,\"progress\":%u,\"devices\":[",
        st.running ? "true" : "false", (unsigned)st.slaveId, (unsigned)st.progress);

    for (uint8_t i = 0; i < count && n < (int)sizeof(buf) - 80; i++) {
        bool hasFw  = fwUpdater.hasFirmware(devs[i].slaveId);
        uint32_t sz = hasFw ? fwUpdater.firmwareSize(devs[i].slaveId) : 0;
        n += snprintf(buf + n, sizeof(buf) - n,
            "%s{\"name\":\"%s\",\"id\":%u,\"hasFw\":%s,\"fwSize\":%lu}",
            i ? "," : "", devs[i].name, (unsigned)devs[i].slaveId,
            hasFw ? "true" : "false", (unsigned long)sz);
    }
    if (n < (int)sizeof(buf) - 3) strcpy(buf + n, "]}");
    req->send(200, "application/json", buf);
  });

  /* POST /fw/upload?id=<slaveId> — upload .hex or .bin */
  server.on("/fw/upload", HTTP_POST,
    [](AsyncWebServerRequest *req) {
      char resp[120];
      snprintf(resp, sizeof(resp), "{\"ok\":%s,\"msg\":\"%s\"}",
               fwUp.ok ? "true" : "false", fwUp.msg);
      req->send(200, "application/json", resp);
    },
    _onFwUpload
  );

  /* POST /api/remote/upload — replace Remote.json; reboot to apply */
  server.on("/api/remote/upload", HTTP_POST,
    [](AsyncWebServerRequest *req) {
      char resp[120];
      snprintf(resp, sizeof(resp), "{\"ok\":%s,\"msg\":\"%s\"}",
               remUp.ok ? "true" : "false", remUp.msg);
      req->send(remUp.ok ? 200 : 400, "application/json", resp);
    },
    _onRemoteUpload
  );

  /* POST /fw/flash?id=<slaveId> — trigger OTA flash task */
  server.on("/fw/flash", HTTP_POST, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("id")) {
      req->send(400, "application/json", "{\"ok\":false,\"msg\":\"missing id\"}");
      return;
    }
    uint8_t slaveId = (uint8_t)req->getParam("id")->value().toInt();
    if (fwUpdater.getStatus().running) {
      req->send(409, "application/json", "{\"ok\":false,\"msg\":\"update already running\"}");
      return;
    }
    bool ok = fwUpdater.startUpdate(slaveId);
    char resp[80];
    snprintf(resp, sizeof(resp), "{\"ok\":%s,\"msg\":\"%s\"}",
             ok ? "true" : "false",
             ok ? "Update started" : "Failed to start update");
    req->send(200, "application/json", resp);
  });

  /* GET + POST /api/devices/scan — registered BEFORE /api/devices to win prefix match.
   * POST: start background bus scan.  GET: poll status/results. */
  server.on("/api/devices/scan", HTTP_ANY, [](AsyncWebServerRequest *req) {
    if (req->method() == HTTP_POST) {
        if (IsBusScanRunning()) {
            req->send(200, "application/json", "{\"ok\":true,\"msg\":\"Already scanning\"}");
            return;
        }
        uint8_t from = (uint8_t)(req->hasParam("from") ? req->getParam("from")->value().toInt() : 1);
        uint8_t to   = (uint8_t)(req->hasParam("to")   ? req->getParam("to")->value().toInt()   : 247);
        if (from < 1)   from = 1;
        if (to   > 247) to   = 247;
        if (from > to)  from = 1;
        StartBusScan(from, to);
        char msg[60];
        snprintf(msg, sizeof(msg), "{\"ok\":true,\"msg\":\"Scan started (addr %u-%u)\"}",
                 (unsigned)from, (unsigned)to);
        req->send(200, "application/json", msg);
    } else {
        AsyncResponseStream *resp = req->beginResponseStream("application/json");
        resp->printf("{\"running\":%s,\"progress\":%u,\"total\":%u,\"addresses\":%s}",
            IsBusScanRunning() ? "true" : "false",
            (unsigned)GetBusScanProgress(),
            (unsigned)GetBusScanTotal(),
            GetBusScanResult());
        req->send(resp);
    }
  });

  /* POST /api/devices/save — registered BEFORE /api/devices to win prefix match. */
  server.on("/api/devices/save", HTTP_POST,
    [](AsyncWebServerRequest *req) {
        if (!_devBuf || _devBufLen == 0) {
            req->send(400, "application/json", "{\"ok\":false,\"msg\":\"No data received\"}");
            return;
        }
        DynamicJsonDocument doc(8192);
        if (deserializeJson(doc, (const char*)_devBuf, _devBufLen)) {
            req->send(400, "application/json", "{\"ok\":false,\"msg\":\"Invalid JSON\"}");
            return;
        }
        File f = LittleFS.open("/Remote.json", "w");
        if (!f) {
            req->send(500, "application/json", "{\"ok\":false,\"msg\":\"FS open failed\"}");
            return;
        }
        f.write((const uint8_t*)_devBuf, _devBufLen);
        f.close();
        Log(NOTIFY, "Web: Remote.json saved via devices tab (%u B)\r\n", (unsigned)_devBufLen);
        req->send(200, "application/json", "{\"ok\":true,\"msg\":\"Saved — reboot to apply\"}");
    },
    nullptr,
    _devBodyHandler
  );

  /* GET /api/devices/status — live verification status per device address.
   * Registered BEFORE /api/devices to win prefix match. */
  server.on("/api/devices/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    const RemoteConfig_t& cfg = GetRemoteConfig();
    AsyncResponseStream *resp = req->beginResponseStream("application/json");
    resp->print("{");
    bool first = true;
    bool seen[8] = {};  // MAX_REMOTE_DEVICES = 8
    for (uint8_t g = 0; g < RemoteGrpCount(); g++) {
        uint8_t di = RemoteGrpDevIdx(g);
        if (di >= cfg.deviceCount || di >= 8 || seen[di]) continue;
        seen[di] = true;
        ModuleStatus_t st = RemoteDevStatus(di);
        if (!first) resp->print(",");
        first = false;
        if (st == MODULE_INVALID) {
            uint16_t got = RemoteGrpDevice(g)->getRaw(0);  // version always at [0]
            resp->printf("\"%u\":{\"status\":\"invalid\",\"expected\":%u,\"got\":%u}",
                (unsigned)cfg.devices[di].address,
                (unsigned)cfg.devices[di].swVersion, (unsigned)got);
        } else if (st == MODULE_TYPE_MISMATCH) {
            uint16_t got = RemoteGrpDevice(g)->getRaw(1);  // typeId always at [1]
            resp->printf("\"%u\":{\"status\":\"type_mismatch\",\"expected\":%u,\"got\":%u}",
                (unsigned)cfg.devices[di].address,
                (unsigned)cfg.devices[di].typeId, (unsigned)got);
        } else {
            const char* s = (st == MODULE_VALID)  ? "valid"   :
                            (st == MODULE_OFFLINE) ? "offline" : "unknown";
            resp->printf("\"%u\":\"%s\"", (unsigned)cfg.devices[di].address, s);
        }
    }
    resp->print("}");
    req->send(resp);
  });

  /* GET /api/devices — serve Remote.json directly; registered last as it prefix-matches all /api/devices/* */
  server.on("/api/devices", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!LittleFS.exists("/Remote.json")) {
        req->send(200, "application/json", "{\"devices\":[]}");
        return;
    }
    req->send(LittleFS, "/Remote.json", "application/json");
  });

  /* GET /api/files — list LittleFS files with sizes + usage stats */
  server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *req) {
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    String json = "{\"total\":" + String(total) +
                  ",\"used\":"  + String(used)  +
                  ",\"files\":[";
    File root = LittleFS.open("/");
    File f    = root.openNextFile();
    bool first = true;
    while (f) {
      if (!f.isDirectory()) {
        if (!first) json += ",";
        json += "{\"name\":\"" + String(f.name()) +
                "\",\"size\":"  + String(f.size()) + "}";
        first = false;
      }
      f = root.openNextFile();
    }
    json += "]}";
    req->send(200, "application/json", json);
  });

  /* DELETE /api/files?name=/Remote.json — delete a file */
  server.on("/api/files", HTTP_DELETE, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("name")) {
      req->send(400, "application/json", "{\"ok\":false,\"msg\":\"missing name\"}");
      return;
    }
    String name = req->getParam("name")->value();
    bool ok = LittleFS.remove(name);
    char resp[80];
    snprintf(resp, sizeof(resp), "{\"ok\":%s,\"msg\":\"%s\"}",
             ok ? "true" : "false",
             ok ? "Deleted" : "Delete failed");
    req->send(200, "application/json", resp);
    Log(ok ? LOG : ERROR, "Web: %s file %s\r\n", ok ? "deleted" : "failed to delete", name.c_str());
  });

  /* POST /api/files/upload?name=/Remote.json — upload any file to LittleFS (multipart) */
  static struct { bool ok = false; bool aborted = false; char msg[80] = {}; char path[64] = {}; } _fileUp;
  server.on("/api/files/upload", HTTP_POST,
    [](AsyncWebServerRequest *req) {
      char resp[120];
      snprintf(resp, sizeof(resp), "{\"ok\":%s,\"msg\":\"%s\"}",
               _fileUp.ok ? "true" : "false", _fileUp.msg);
      req->send(_fileUp.ok ? 200 : 400, "application/json", resp);
    },
    [](AsyncWebServerRequest *req, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0) {
        _fileUp.ok = false; _fileUp.aborted = false; _fileUp.msg[0] = '\0';
        String name = req->hasParam("name") ? req->getParam("name")->value() : "/" + filename;
        if (!name.startsWith("/")) name = "/" + name;
        strlcpy(_fileUp.path, name.c_str(), sizeof(_fileUp.path));
        Log(NOTIFY, "Web: uploading → %s\r\n", _fileUp.path);
      }
      if (_fileUp.aborted) return;
      File f = LittleFS.open(_fileUp.path, index == 0 ? "w" : "a");
      if (!f) { _fileUp.aborted = true; strlcpy(_fileUp.msg, "FS open failed", sizeof(_fileUp.msg)); return; }
      if (f.write(data, len) != len) { _fileUp.aborted = true; strlcpy(_fileUp.msg, "Write failed — full?", sizeof(_fileUp.msg)); }
      f.close();
      if (final && !_fileUp.aborted) {
        _fileUp.ok = true;
        snprintf(_fileUp.msg, sizeof(_fileUp.msg), "%s saved (%u B)", _fileUp.path, (unsigned)(index + len));
        Log(NOTIFY, "Web: %s\r\n", _fileUp.msg);
      }
    }
  );

  /* GET /log — recent log ring buffer as plain text, newest last */
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest* req) {
    uint8_t count = LogRingCount();
    String out;
    out.reserve((size_t)count * 80);
    for (uint8_t i = 0; i < count; i++) {
      out += LogRingGet(i);
      out += '\n';
    }
    req->send(200, "text/plain", out);
  });

  /* ── Static file serving and root page ───────────────────────────────── */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Log(DEBUG,"MP server\r\n");
    request->send(LittleFS, "/Main.html", "text/html");
  });
  server.on("/ESP32PLC.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/ESP32PLC.css", "text/css");
  });
  server.serveStatic("/", LittleFS, "/");

  server.onNotFound([](AsyncWebServerRequest *request){
    if (_captive) {
      // Unknown URL in AP mode — redirect to portal (covers remaining OS probes)
      request->redirect("http://192.168.4.1/");
      return;
    }
    Log(DEBUG,"Bad Request: %s\r\n",request->url());
    request->send(404);
  });
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
}

void WebHandel(){
  if((millis() - LastTime) > 2000){
    LastTime = millis();
    if(wsconnected == true){
      lastButtonState = digitalRead(USER_SW);
      jsonDocTx.clear();
      //jsonDocTx["SSID"] = GetSSID();
      //jsonDocTx["IP"] = GetIPStr();
      //jsonDocTx["HN"] = GetHostName();
      //jsonDocTx["RSSI"] = GetRSSIStr();
      //jsonDocTx["MAC"] = GetMACStr();
      ///jsonDocTx["Temp"] = String(getDeviceClimateTemprature());
      //jsonDocTx["Humid"] = String(getDeviceClimateHumidity());
      //jsonDocTx["button"] = lastButtonState;
      //jsonDocTx["Input1"] = lastButtonState;
      //jsonDocTx["Input2"] = lastButtonState;
      //jsonDocTx["Input3"] = lastButtonState;
      //jsonDocTx["Input4"] = lastButtonState;
      //jsonDocTx["Output1"] = lastButtonState;
      //jsonDocTx["Output2"] = lastButtonState;
      //jsonDocTx["Output3"] = lastButtonState;
      //jsonDocTx["Output4"] = lastButtonState;

      serializeJson(jsonDocTx, output, OUTPUT_BUF_SIZE);

      Log(DEBUG,"Sending Data\r\n");
      if (ws.availableForWriteAll()) {
        ws.textAll(output);
        Log(DEBUG,"Data Sent\r\n");
      } 
      else {
        Log(DEBUG,"Data que\r\n");
      }
    }
  }
}

char WebLogSend(String LogString){
  if(wsconnected == true){
    //Serial.println(LogString);
    jsonDocTx.clear();
    jsonDocTx["Type"] = 10; //Log Send Command
    jsonDocTx["LOG"] = LogString + "\n";//\n";
    serializeJson(jsonDocTx, output, OUTPUT_BUF_SIZE);
    if (ws.availableForWriteAll()) {
      ws.textAll(output);
        //Log(NOTIFY,"Sent Log");
    } 
/*       else {
        Log(ERROR,"Queue Is Full");
      } */
    return 1;
  }
  return 0;
}

void WSRunJSON(){
  if(jsonDocRx["Mode"] == "FW"){
    SetFWData(jsonDocRx["CH1"],jsonDocRx["CH2"],jsonDocRx["CH3"],jsonDocRx["CH4"],jsonDocRx["CH5"],int(jsonDocRx["CH1T"]), int(jsonDocRx["CH2T"]),int(jsonDocRx["CH3T"]),int(jsonDocRx["CH4T"]),int(jsonDocRx["CH5T"]));
  }
}
