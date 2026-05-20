#include "MQTT.h"
#include "Functions.h"
#include <ArduinoJson.h>
#include "Display/Oled.h"
#include "Sensors.h"
#include <PubSubClient.h>
#include "FileSystem/FSInterface.h"
#include "Remote/MasterController.h"
#include <RemoteDeviceConfig.h>
#include "Devices/Log.h"

#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];
char MQTTActive  = 0;
char MQTTLockout = 0;
char temp[50];
char ErrorCounter = 0;

/* Persistent server IP buffer — PubSubClient keeps a pointer, must outlive setServer() */
static char _mqttServerBuf[40] = {};

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

void callback(char* topic, byte* payload, unsigned int length) {
    String messageTemp;
    for (unsigned int i = 0; i < length; i++) messageTemp += (char)payload[i];

    Log(NOTIFY, "MQTT rx [%s] %s\r\n", topic, messageTemp.c_str());

    if (_handleRemoteWrite(topic, messageTemp.c_str())) return;

    char t[80];
    snprintf(t, sizeof(t), "%s/Heater", _mqttBase);
    if (strcmp(topic, t) == 0) {
        bool on = messageTemp == "on";
        digitalWrite(40, on ? HIGH : LOW);
        snprintf(t, sizeof(t), "%s/Heater/State", _mqttBase);
        client.publish(t, on ? "on" : "off");
        return;
    }
    snprintf(t, sizeof(t), "%s/Lights", _mqttBase);
    if (strcmp(topic, t) == 0) {
        bool on = messageTemp == "on";
        digitalWrite(41, on ? HIGH : LOW);
        digitalWrite(42, on ? HIGH : LOW);
        digitalWrite(39, on ? HIGH : LOW);
        snprintf(t, sizeof(t), "%s/Lights/State", _mqttBase);
        client.publish(t, on ? "on" : "off");
        return;
    }
}

void MqttLoop(void) {
    if (MQTTLockout) {
        if (ErrorCounter > 20) ESP.restart();
        return;
    }
    if (client.connected()) {
        client.loop();
    } else {
        MQTTreconnect();
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
    snprintf(_mqttBase, sizeof(_mqttBase), "ESPPLC/%s", GetHostName().c_str());
    uint16_t port = GetMQTTPort();
    Log(LOG, "MQTT: starting — server=%s port=%u base=%s\r\n", _mqttServerBuf, port, _mqttBase);
    client.setServer(_mqttServerBuf, port);
    client.setCallback(callback);
}

void MQTTreconnect(void) {
    if (!GetMQTTEnabled() || MQTTLockout) return;
    if (GetWiFiStatus() != 1) return;

    char counter = 0;
    String user = GetMQTTUser();
    String pass = GetMQTTPassword();

    while (!client.connected()) {
        Log(NOTIFY, "MQTT: connecting...\r\n");
        if (client.connect(GetClientId().c_str(), user.c_str(), pass.c_str())) {
            MQTTActive = 1;
            Log(LOG, "MQTT: connected\r\n");
            char subTopic[128];
            snprintf(subTopic, sizeof(subTopic), "%s/Heater", _mqttBase);
            client.subscribe(subTopic);

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
        }
        counter++;
        if (counter > 5) {
            Log(ERROR, "MQTT: connection failed — locking out\r\n");
            MQTTLockout = 1;
            break;
        }
        delay(200);
    }
}

void SetMQTTLockout(char Mode) {
    MQTTLockout = Mode ? 1 : 0;
}

void SendDeviceEnviroment() {
    if (!MQTTActive) return;
    readDeviceClimate();
    printInfo();
    String TempMesure = String(getDeviceClimateTemprature());
    TempMesure.toCharArray(temp, TempMesure.length() + 1);
    client.publish("home/garage/cf/device/temp", temp);
    TempMesure = String(getDeviceClimateHumidity());
    TempMesure.toCharArray(temp, TempMesure.length() + 1);
    client.publish("home/garage/cf/device/humid", temp);
}


// ----------------------------------------------------------------
// SendRemoteDevices — publish all JSON-configured device groups
// Each enabled group publishes: {mqttTopic}/{regName} = value
// ----------------------------------------------------------------
void SendRemoteDevices() {
    if (!MQTTActive) return;

    const RemoteConfig_t& cfg = GetRemoteConfig();

    for (uint8_t gi = 0; gi < RemoteGrpCount(); gi++) {
        uint8_t        di  = RemoteGrpDevIdx(gi);
        uint8_t        gri = RemoteGrpGrpIdx(gi);
        ModbusDevice*  dev = RemoteGrpDevice(gi);

        if (!dev || RemoteDevStatus(di) != MODULE_VALID) continue;
        if (di >= cfg.deviceCount) continue;

        const RemoteGroupCfg_t& grp = cfg.devices[di].groups[gri];
        if (!grp.mqttEnable || grp.mqttTopic[0] == '\0') continue;

        for (uint8_t r = 0; r < grp.count; r++) {
            float val = (grp.scale == 1.0f)
                ? (float)dev->getRaw(r)
                : (float)dev->getSigned(r) / grp.scale;

            char topic[128];
            if (grp.regs[r][0])
                snprintf(topic, sizeof(topic), "%s/%s/%s", _mqttBase, grp.mqttTopic, grp.regs[r]);
            else
                snprintf(topic, sizeof(topic), "%s/%s/reg%u", _mqttBase, grp.mqttTopic, r);

            char buf[20];
            snprintf(buf, sizeof(buf), "%.2f", val);
            if (!client.publish(topic, buf)) ErrorCounter++;
            else ErrorCounter = 0;
        }
    }
}

char GetMQTTStatus(void) {
    return MQTTActive;
}
