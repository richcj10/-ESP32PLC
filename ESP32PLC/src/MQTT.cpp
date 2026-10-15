#include "MQTT.h"
#include "Functions.h"
#include <ArduinoJson.h>
#include "Sensors.h"
#include <PubSubClient.h>
#include "FileSystem/FSInterface.h"
#include "Remote/MasterController.h"
#include <RemoteDeviceConfig.h>
#include "Devices/Log.h"
#include <esp_heap_caps.h>
#include "HAL/Digital/Digital.h"
#include "Devices/LEDStrip.h"
#include "Modules/ModuleManager.h"
#include "Remote/FwUpdater.h"
#include <WiFi.h>

char MQTTActive  = 0;
char MQTTLockout = 0;

// SendLocalIO state — on-change publish + hourly heartbeat (see SendLocalIO)
#define IO_HEARTBEAT_MS  (60UL * 60UL * 1000UL)   // 1 hour
#define IO_MAX_INPUTS    32
static int8_t        _ioLast[IO_MAX_INPUTS];
static unsigned long _ioLastBeat = 0;
static bool          _ioForceAll = true;           // set on (re)connect → publish all

/* Persistent server IP buffer — PubSubClient keeps a pointer, must outlive setServer() */
static char _mqttServerBuf[40] = {};
static char _mqttClientId[24]  = "ESPPLC";   // set in MQTTStart() from the full MAC

/* Base topic: "ESPPLC/<hostname>" — built once at MQTTStart, reused everywhere */
static char _mqttBase[48] = "ESPPLC";

const char* GetMQTTBaseTopic() { return _mqttBase; }

WiFiClient wclient;
PubSubClient client(wclient);

PubSubClient& GetMQTTClient() { return client; }

// ----------------------------------------------------------------
// Remote write — look up device/writeGroup by topic, parse JSON
// payload keyed by reg name, fill defaults for missing keys, queue.
// ----------------------------------------------------------------
static bool _handleRemoteWrite(const char* topic, const char* payload) {
    const RemoteConfig_t& cfg = GetRemoteConfig();

    for (uint8_t di = 0; di < cfg.deviceCount; di++) {
        const RemoteDeviceCfg_t& dev = cfg.devices[di];
        for (uint8_t gi = 0; gi < dev.groupCount; gi++) {
        for (uint8_t wi = 0; wi < dev.groups[gi].writeCount; wi++) {
            const RemoteWriteGroupCfg_t& wg = dev.groups[gi].writes[wi];
            char fullTopic[128];
            snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _mqttBase, wg.mqttTopic);
            if (strcmp(topic, fullTopic) != 0) continue;

            StaticJsonDocument<256> doc;
            if (deserializeJson(doc, payload) != DeserializationError::Ok) {
                Log(ERROR, "MQTT write [%s]: bad JSON\r\n", wg.name);
                return true;  // topic matched — stop searching
            }

            uint16_t vals[MAX_REGS_PER_GROUP];
            for (uint8_t r = 0; r < wg.count && r < MAX_REGS_PER_GROUP; r++) {
                if (wg.regs[r][0] && doc.containsKey(wg.regs[r]))
                    vals[r] = (uint16_t)doc[wg.regs[r]].as<int>();
                else
                    vals[r] = wg.defaults[r];
            }

            bool ok = false;
            if (wg.fc == 5 && wg.count == 1)
                ok = master.queueWriteCoil(dev.address, wg.startReg, vals[0] != 0);
            else if (wg.fc == 6 && wg.count == 1)
                ok = master.queueWrite(dev.address, wg.startReg, vals[0]);
            else
                ok = master.queueWriteMulti(dev.address, wg.startReg, vals, wg.count);

            Log(ok ? LOG : ERROR, "MQTT write [%s.%s] addr=%u reg=%u count=%u %s\r\n",
                dev.name, wg.name, dev.address, wg.startReg, wg.count,
                ok ? "queued" : "queue full");
            return true;
        }
        }  // gi
    }
    return false;
}

// ----------------------------------------------------------------
// LED strip state publish — call after any color change
// ----------------------------------------------------------------
static void _publishStripState() {
    if (!MQTTActive) return;
    char topic[96];
    char buf[72];
    snprintf(topic, sizeof(topic), "%s/light/strip/state", _mqttBase);
    if (!ledStrip.isOn()) {
        strlcpy(buf, "{\"state\":\"OFF\"}", sizeof(buf));
    } else {
        snprintf(buf, sizeof(buf),
            "{\"state\":\"ON\",\"color\":{\"r\":%u,\"g\":%u,\"b\":%u},\"brightness\":%u}",
            ledStrip.r(), ledStrip.g(), ledStrip.b(), ledStrip.brightness());
    }
    client.publish(topic, buf);
}

// LED strip write — handles ESPPLC/<host>/light/strip/set
// HA JSON schema: {"state":"ON","color":{"r":255,"g":0,"b":0},"brightness":255}
// ----------------------------------------------------------------
static bool _handleStripWrite(const char* topic, const char* payload) {
    char expected[96];
    snprintf(expected, sizeof(expected), "%s/light/strip/set", _mqttBase);
    if (strcmp(topic, expected) != 0) return false;

    StaticJsonDocument<192> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return true;

    const char* state = doc["state"] | "ON";
    if (strcasecmp(state, "OFF") == 0) {
        ledStrip.setOff();
    } else {
        uint8_t r   = doc["color"]["r"]  | ledStrip.r();
        uint8_t g   = doc["color"]["g"]  | ledStrip.g();
        uint8_t b   = doc["color"]["b"]  | ledStrip.b();
        uint8_t bri = doc["brightness"]  | ledStrip.brightness();
        ledStrip.setColor(r, g, b, bri);
    }
    _publishStripState();
    return true;
}

// Local I/O write — handles ESPPLC/<host>/io/out/<n>
// payload "1"/"on" → HIGH, anything else → LOW
// ----------------------------------------------------------------
static bool _handleIOWrite(const char* topic, const char* payload) {
    char prefix[80];
    snprintf(prefix, sizeof(prefix), "%s/io/out/", _mqttBase);
    size_t plen = strlen(prefix);
    if (strncmp(topic, prefix, plen) != 0) return false;
    uint8_t n   = (uint8_t)atoi(topic + plen);
    bool    val = (strcmp(payload, "1") == 0 ||
                   strcasecmp(payload, "on") == 0);
    SetOutput(n, val);
    Log(LOG, "MQTT: io/out/%u = %s\r\n", (unsigned)n, val ? "on" : "off");
    return true;
}

void callback(char* topic, byte* payload, unsigned int length) {
    String messageTemp;
    for (unsigned int i = 0; i < length; i++) messageTemp += (char)payload[i];

    Log(NOTIFY, "MQTT rx [%s] %s\r\n", topic, messageTemp.c_str());

    if (_handleRemoteWrite(topic, messageTemp.c_str()))       return;
    if (_handleIOWrite(topic, messageTemp.c_str()))           return;
    if (_handleStripWrite(topic, messageTemp.c_str()))        return;
    if (modules.handleMQTT(topic, messageTemp.c_str()))       return;

}

// ── Health and reboot policy ────────────────────────────────────────────────
// Retrying forever handles a broker restart or a WiFi blip. If MQTT still can't
// stay up, reboot — but only when a reboot can help:
//   - no connection for MQTT_DOWN_REBOOT_MS straight while WiFi is up, or
//   - MQTT_FLAP_LIMIT drops within MQTT_FLAP_WINDOW_MS (connects, then keeps dropping)
// Never for bad credentials (state 4/5): a reboot can't fix those — shown as
// "Auth failed" instead. Never mid firing sequence or device firmware job.
#define MQTT_DOWN_REBOOT_MS  (15UL * 60UL * 1000UL)
#define MQTT_FLAP_WINDOW_MS  (30UL * 60UL * 1000UL)
#define MQTT_FLAP_LIMIT      8

static unsigned long _mqttDownSince = 0;     // start of the current outage, 0 = up
static unsigned long _flapStart     = 0;
static uint8_t       _flapCount     = 0;
static bool          _wasConnected  = false;
static int           _lastConnState = 0;     // PubSubClient state of the last failed connect
static const char*   _rebootReason  = nullptr;

static const char* _stateText(int s) {
    switch (s) {
        case -4: return "timeout";            case -3: return "connection lost";
        case -2: return "broker unreachable"; case -1: return "disconnected";
        case  1: return "bad protocol";       case  2: return "client ID rejected";
        case  3: return "broker unavailable"; case  4: return "bad username/password";
        case  5: return "not authorized";     default: return "unknown";
    }
}
static bool _authFailed() { return _lastConnState == 4 || _lastConnState == 5; }

static void _requestReboot(const char* why) {
    if (!_rebootReason) {
        _rebootReason = why;
        Log(ERROR, "MQTT: %s - rebooting%s", why,
            (fwUpdater.getStatus().running || modules.anyBusy()) ? " when the current job finishes" : "");
    }
}

void MqttLoop(void) {
    if (MQTTLockout) return;

    if (_rebootReason && !fwUpdater.getStatus().running && !modules.anyBusy()) {
        Log(NOTIFY_FORCE, "MQTT: rebooting (%s)", _rebootReason);
        delay(500);
        ESP.restart();
    }

    if (client.connected()) {
        _wasConnected  = true;
        _mqttDownSince = 0;
        client.loop();
        return;
    }

    MQTTActive = 0;   // stop publishers until reconnected
    unsigned long now = millis();
    if (_wasConnected) {                       // it was up and dropped
        _wasConnected = false;
        if (!_flapStart || now - _flapStart > MQTT_FLAP_WINDOW_MS) { _flapStart = now; _flapCount = 0; }
        _flapCount++;
        Log(ERROR, "MQTT: connection lost (%s) - drop %u of %u allowed in 30 min",
            _stateText(client.state()), (unsigned)_flapCount, (unsigned)MQTT_FLAP_LIMIT);
        if (_flapCount >= MQTT_FLAP_LIMIT) _requestReboot("connection keeps dropping");
    }

    MQTTreconnect();

    // Outage clock: only while WiFi is really up and it isn't a credentials problem
    if (WiFi.status() == WL_CONNECTED && !_authFailed()) {
        if (!_mqttDownSince) _mqttDownSince = now;
        else if (now - _mqttDownSince > MQTT_DOWN_REBOOT_MS) _requestReboot("no broker connection for 15 min");
    } else {
        _mqttDownSince = 0;
    }
}

void MQTTStart() {
    if (!GetMQTTEnabled()) {
        Log(NOTIFY, "MQTT: disabled in config — skipping\r\n");
        MQTTLockout = 1;
        return;
    }
    String ip = GetMQTTIP();
    if (ip.length() == 0) {
        Log(ERROR, "MQTT: no server IP configured — MQTT disabled\r\n");
        MQTTLockout = 1;
        return;
    }
    strlcpy(_mqttServerBuf, ip.c_str(), sizeof(_mqttServerBuf));
    // Broker-facing client ID: full MAC in hex, unique per device. The old ID
    // (GetClientId: last two MAC bytes in DECIMAL, run together) could collide —
    // e.g. 18,152 and 181,52 both give "ESPPLC-18152" — and a broker kicks the
    // older of two same-ID clients, so two such PLCs knock each other off forever.
    {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(_mqttClientId, sizeof(_mqttClientId), "ESPPLC-%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    snprintf(_mqttBase, sizeof(_mqttBase), "ESPPLC/%s", GetHostName().c_str());
    uint16_t port = GetMQTTPort();
    Log(LOG, "MQTT: starting — server=%s port=%u base=%s id=%s\r\n",
        _mqttServerBuf, port, _mqttBase, _mqttClientId);
    client.setBufferSize(512);
    client.setSocketTimeout(2);   // was PubSubClient's 15 s default — bounds each attempt
    client.setServer(_mqttServerBuf, port);
    client.setCallback(callback);
}

// One connect attempt per call, rate-limited: every 5 s for the first few misses,
// then every 30 s — and it never gives up for good. (It used to try 6 times back
// to back with the 15 s socket timeout, freezing loop() for up to a minute and a
// half, then lock MQTT out until reboot.)
static unsigned long _mqttLastTry = 0;
static uint8_t       _mqttFails   = 0;
#define MQTT_RETRY_FAST_MS   5000UL
#define MQTT_RETRY_SLOW_MS  30000UL

void MQTTreconnect(void) {
    if (!GetMQTTEnabled() || MQTTLockout) return;
    if (GetWiFiStatus() != 1) return;
    unsigned long wait = (_mqttFails < 3) ? MQTT_RETRY_FAST_MS : MQTT_RETRY_SLOW_MS;
    if (_mqttLastTry && millis() - _mqttLastTry < wait) return;
    _mqttLastTry = millis();

    String user = GetMQTTUser();
    String pass = GetMQTTPassword();

    {
        Log(NOTIFY, "MQTT: connecting...\r\n");
        if (client.connect(_mqttClientId, user.c_str(), pass.c_str())) {
            _mqttFails     = 0;
            _lastConnState = 0;
            MQTTActive = 1;
            _ioForceAll = true;   // republish all inputs after (re)connect
            Log(LOG, "MQTT: connected\r\n");
            char subTopic[128];

            /* Subscribe to local I/O output control topics */
            for (uint8_t n = 0; n < GetOutputCount(); n++) {
                snprintf(subTopic, sizeof(subTopic), "%s/io/out/%u", _mqttBase, (unsigned)n);
                client.subscribe(subTopic);
                Log(LOG, "MQTT: subscribed io/out/%u\r\n", (unsigned)n);
            }

            /* Subscribe to LED strip control */
            snprintf(subTopic, sizeof(subTopic), "%s/light/strip/set", _mqttBase);
            client.subscribe(subTopic);
            Log(LOG, "MQTT: subscribed light/strip/set\r\n");

            // Auto-subscribe to all write topics nested inside groups
            const RemoteConfig_t& cfg = GetRemoteConfig();
            for (uint8_t di = 0; di < cfg.deviceCount; di++) {
                const RemoteDeviceCfg_t& dev = cfg.devices[di];
                for (uint8_t gi = 0; gi < dev.groupCount; gi++) {
                    for (uint8_t wi = 0; wi < dev.groups[gi].writeCount; wi++) {
                        const char* t = dev.groups[gi].writes[wi].mqttTopic;
                        if (t[0]) {
                            snprintf(subTopic, sizeof(subTopic), "%s/%s", _mqttBase, t);
                            client.subscribe(subTopic);
                            Log(LOG, "MQTT: subscribed [%s/%s/%s] → %s\r\n",
                                dev.name, dev.groups[gi].name,
                                dev.groups[gi].writes[wi].name, subTopic);
                        }
                    }
                }
            }
            PublishHADiscovery();
            modules.onMQTTConnect();
        } else {
            if (_mqttFails < 255) _mqttFails++;
            _lastConnState = client.state();
            Log(ERROR, "MQTT: connect to %s:%u failed: %s (state %d)%s - retry in %lus\r\n",
                _mqttServerBuf, (unsigned)GetMQTTPort(), _stateText(_lastConnState), _lastConnState,
                _authFailed() ? " - check MQTT user/password" : "",
                (_mqttFails < 3 ? MQTT_RETRY_FAST_MS : MQTT_RETRY_SLOW_MS) / 1000);
        }
    }
}

void SetMQTTLockout(char Mode) {
    MQTTLockout = Mode ? 1 : 0;
}

// ----------------------------------------------------------------
// HA MQTT Discovery — publishes homeassistant/sensor/<uid>/config
// for every mqttEnable=true register. Called once after connect.
// Uses abbreviated HA keys to keep payloads within 512 bytes.
// ----------------------------------------------------------------
static void _sanitizeId(const char* in, char* out, size_t len) {
    size_t i = 0;
    for (; *in && i < len - 1; in++, i++) {
        char c = *in;
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c == ' ' || c == '/' || c == '.' || c == '-') c = '_';
        out[i] = c;
    }
    out[i] = '\0';
}

void PublishHADiscovery() {
    if (!MQTTActive) return;
    const RemoteConfig_t& cfg = GetRemoteConfig();
    if (!cfg.loaded || cfg.deviceCount == 0) return;

    char hostSafe[24];
    _sanitizeId(GetHostName().c_str(), hostSafe, sizeof(hostSafe));

    char payload[512];
    char discTopic[128];

    for (uint8_t di = 0; di < cfg.deviceCount; di++) {
        const RemoteDeviceCfg_t& dev = cfg.devices[di];
        char devSafe[24]; _sanitizeId(dev.name, devSafe, sizeof(devSafe));
        char devId[52];
        snprintf(devId, sizeof(devId), "espplc_%s_%s", hostSafe, devSafe);

        for (uint8_t gi = 0; gi < dev.groupCount; gi++) {
            const RemoteGroupCfg_t& grp = dev.groups[gi];
            if (!grp.mqttEnable || grp.mqttTopic[0] == '\0') continue;

            for (uint8_t ri = 0; ri < grp.count; ri++) {
                const char* rname = (grp.regs[ri][0] != '\0') ? grp.regs[ri] : nullptr;
                if (!rname) continue;

                char rSafe[24]; _sanitizeId(rname, rSafe, sizeof(rSafe));
                char uid[76];
                snprintf(uid, sizeof(uid), "%s_%s", devId, rSafe);

                char statTopic[128];
                snprintf(statTopic, sizeof(statTopic), "%s/%s/%s",
                         _mqttBase, grp.mqttTopic, rname);

                snprintf(discTopic, sizeof(discTopic),
                         "homeassistant/sensor/%s/config", uid);

                int n = snprintf(payload, sizeof(payload),
                    "{\"name\":\"%s\","
                    "\"stat_t\":\"%s\","
                    "\"unit_of_meas\":\"%s\","
                    "\"state_class\":\"measurement\","
                    "\"uniq_id\":\"%s\","
                    "\"val_tpl\":\"{{value|float}}\","
                    "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"ESP32PLC\"}}",
                    rname, statTopic, grp.units[ri], uid, devId, dev.name);

                if (n > 0 && n < (int)sizeof(payload)) {
                    client.publish(discTopic, (const uint8_t*)payload, (unsigned int)n, true);
                    Log(LOG, "HA disc: %s\r\n", discTopic);
                }
            }
        }
    }

    // LED strip light entity discovery
    {
        char uid[52];
        snprintf(uid, sizeof(uid), "espplc_%s_strip", hostSafe);
        char ltopic[96];
        snprintf(ltopic, sizeof(ltopic), "homeassistant/light/%s/config", uid);
        int n = snprintf(payload, sizeof(payload),
            "{\"name\":\"LED Strip\","
            "\"schema\":\"json\","
            "\"cmd_t\":\"%s/light/strip/set\","
            "\"stat_t\":\"%s/light/strip/state\","
            "\"rgb\":true,"
            "\"bri\":true,"
            "\"uniq_id\":\"%s\","
            "\"dev\":{\"ids\":[\"espplc_%s\"],\"name\":\"ESP32PLC\",\"mf\":\"ESP32PLC\"}}",
            _mqttBase, _mqttBase, uid, hostSafe);
        if (n > 0 && n < (int)sizeof(payload))
            client.publish(ltopic, (const uint8_t*)payload, (unsigned int)n, true);
    }
}



// ----------------------------------------------------------------
// SendRemoteDevices — publish all JSON-configured device groups
// Each enabled group publishes: {mqttTopic}/{regName} = value
// ----------------------------------------------------------------
// Minimum value change that triggers an MQTT publish (engineering units after scaling).
#define MQTT_PUBLISH_THRESHOLD 0.25f

// Publish-on-change cache — NAN sentinel forces publish on first call.
// Indexed by group-pool index (gi) and register offset (r).
// Lives in PSRAM (7.5 KB) — allocated on first publish.
static float  (*_lastPubVal)[MAX_REGS_PER_GROUP] = nullptr;
static uint32_t _lastPubMs[MAX_GROUP_POOL];

void SendRemoteDevices() {
    if (!MQTTActive) return;

    if (!_lastPubVal) {
        size_t sz = sizeof(float) * MAX_GROUP_POOL * MAX_REGS_PER_GROUP;
        _lastPubVal = (float(*)[MAX_REGS_PER_GROUP])heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!_lastPubVal) _lastPubVal = (float(*)[MAX_REGS_PER_GROUP])malloc(sz);
        if (!_lastPubVal) return;
        for (uint8_t i = 0; i < MAX_GROUP_POOL; i++)
            for (uint8_t r = 0; r < MAX_REGS_PER_GROUP; r++)
                _lastPubVal[i][r] = NAN;
    }

    const RemoteConfig_t& cfg = GetRemoteConfig();

    for (uint8_t gi = 0; gi < RemoteGrpCount(); gi++) {
        uint8_t        di  = RemoteGrpDevIdx(gi);
        uint8_t        gri = RemoteGrpGrpIdx(gi);
        ModbusDevice*  dev = RemoteGrpDevice(gi);

        if (!dev || RemoteDevStatus(di) != MODULE_VALID) continue;
        if (di >= cfg.deviceCount) continue;

        const RemoteGroupCfg_t& grp = cfg.devices[di].groups[gri];
        if (!grp.mqttEnable || grp.mqttTopic[0] == '\0') continue;

        bool heartbeat = (millis() - _lastPubMs[gi]) >= 60000UL;
        bool published = false;

        for (uint8_t r = 0; r < grp.count; r++) {
            float val = (grp.scale == 1.0f)
                ? (float)dev->getRaw(r)
                : (float)dev->getSigned(r) / grp.scale;

            bool changed = isnan(_lastPubVal[gi][r]) || fabsf(val - _lastPubVal[gi][r]) > MQTT_PUBLISH_THRESHOLD;
            if (!changed && !heartbeat) continue;

            char topic[128];
            if (grp.regs[r][0])
                snprintf(topic, sizeof(topic), "%s/%s/%s", _mqttBase, grp.mqttTopic, grp.regs[r]);
            else
                snprintf(topic, sizeof(topic), "%s/%s/reg%u", _mqttBase, grp.mqttTopic, r);

            char buf[20];
            snprintf(buf, sizeof(buf), "%.2f", val);
            client.publish(topic, buf);

            _lastPubVal[gi][r] = val;
            published = true;
        }

        if (published) _lastPubMs[gi] = millis();
    }
}

char GetMQTTStatus(void) {
    return MQTTActive;
}

// Live state for the web UI: "disabled" | "connected" | "connecting" | "ap_mode" | "failed"
const char* GetMQTTState(void) {
    if (!GetMQTTEnabled())        return "disabled";
    if (client.connected())       return "connected";
    if (GetWiFiStatus() != 1)     return "ap_mode";   // running its own AP — no route to broker
    if (MQTTLockout)              return "failed";
    if (_authFailed())            return "auth_failed";   // still retrying, but needs new credentials
    if (_mqttFails >= 3)          return "failed";        // still retrying every 30 s
    return "connecting";
}

// ----------------------------------------------------------------
// SendLocalIO — publishes shield input states
// Topic: ESPPLC/<host>/io/in/<n>   Payload: "0" or "1"
// Publishes an input only when it changes, plus all inputs on every
// (re)connect and as an hourly heartbeat. Cheap enough to call every loop.
// ----------------------------------------------------------------
void SendLocalIO() {
    if (!MQTTActive) return;

    bool all = _ioForceAll || (millis() - _ioLastBeat >= IO_HEARTBEAT_MS);
    if (all) { _ioForceAll = false; _ioLastBeat = millis(); }

    char topic[80];
    char buf[2] = {0, 0};
    uint8_t cnt = GetInputCount();
    if (cnt > IO_MAX_INPUTS) cnt = IO_MAX_INPUTS;
    for (uint8_t n = 0; n < cnt; n++) {
        int8_t v = GetInput(n) ? 1 : 0;
        if (!all && v == _ioLast[n]) continue;
        _ioLast[n] = v;
        snprintf(topic, sizeof(topic), "%s/io/in/%u", _mqttBase, (unsigned)n);
        buf[0] = v ? '1' : '0';
        client.publish(topic, buf);
    }
}
