#include "MQTT.h"
#include "Functions.h"
#include "Display/Oled.h"
#include "Sensors.h"
#include <PubSubClient.h>
#include "FileSystem/FSInterface.h"
#include "Remote/MasterController.h"
#include "Devices/Log.h"

#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];
char MQTTActive  = 0;
char MQTTLockout = 0;
char temp[50];
char ErrorCounter = 0;

/* Persistent server IP buffer — PubSubClient keeps a pointer, must outlive setServer() */
static char _mqttServerBuf[40] = {};

WiFiClient wclient;
PubSubClient client(wclient);

PubSubClient& GetMQTTClient() { return client; }

void callback(char* topic, byte* payload, unsigned int length) {
    String messageTemp;
    for (unsigned int i = 0; i < length; i++) messageTemp += (char)payload[i];

    Log(NOTIFY, "MQTT rx [%s] %s\r\n", topic, messageTemp.c_str());

    if (String(topic) == "ESPPLC/Heater") {
        bool on = messageTemp == "on";
        digitalWrite(40, on ? HIGH : LOW);
        client.publish("ESPPLC/Heater/State", on ? "on" : "off");
    }
    if (String(topic) == "ESPPLC/Lights") {
        bool on = messageTemp == "on";
        digitalWrite(41, on ? HIGH : LOW);
        digitalWrite(42, on ? HIGH : LOW);
        digitalWrite(39, on ? HIGH : LOW);
        client.publish("ESPPLC/Lights/State", on ? "on" : "off");
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
    uint16_t port = GetMQTTPort();
    Log(LOG, "MQTT: starting — server=%s port=%u\r\n", _mqttServerBuf, port);
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
            client.subscribe("ESPPLC/Heater");
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

void SendOutsideEnvoroment() {
    if (!MQTTActive) return;
    String report;
    report = String(GetRemoteDataFromQue(OUTSIDE_TEMP_POS, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Weather/temp", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(OUTSIDE_HUMID_POS, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Weather/humid", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(MC_POS, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Weather/mH", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(WIND_SPEED_POS, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Weather/WindSpeed", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(WIND_DIR_POS, 0));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Weather/windDir", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(RAIN_POS, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Weather/rain", temp)) ErrorCounter++;
    else ErrorCounter = 0;
}

void SendRemoteRTD() {
    if (!MQTTActive) return;
    String report;
    report = String(GetRemoteDataFromQue(REMOTE_TEMP_RTD_1, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/UnderRV/RTD1", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(REMOTE_TEMP_RTD_2, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/UnderRV/RTD2", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(REMOTE_TEMP_RTD_3, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/UnderRV/RTD3", temp)) ErrorCounter++;
}

void SendRemoteCurrentSense() {
    if (!MQTTActive) return;
    String report;
    report = String(GetRemoteDataFromQue(REMOTE_CS_A, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Current/A", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(REMOTE_CS_B, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Current/B", temp)) ErrorCounter++;
    report = String(GetRemoteDataFromQue(REMOTE_CS_C, 1));
    report.toCharArray(temp, report.length() + 1);
    if (!client.publish("Outside/Current/C", temp)) ErrorCounter++;
}

char GetMQTTStatus(void) {
    return MQTTActive;
}
