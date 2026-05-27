#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "WifiControl/WifiConfig.h"
#include "FileSystem/FSInterface.h"
#include "Define.h"
#include "Functions.h"
#include "Devices/StatusLED.h"
#include "Devices/Log.h"
#include "Display/Display.h"
#include "MQTT.h"

static String IpAddress2String(const IPAddress& ipAddress);

#define WIFI_STA_TIMEOUT_MS     30000UL              // 30s total STA connect window
#define WIFI_AP_RECOVERY_MS     (5UL * 60UL * 1000UL) // 5 min AP before retrying STA

static bool          _inAPRecovery    = false;
static unsigned long _apRecoveryStart = 0;

static void startAP(bool recovery);

char SetupWiFi(void) {
    char mode = (char)GetWiFiMode();

    if (mode == WIFI_STA_MODE) {
        String ssid = GetSSID();
        String pass = GetSSIDPassword();

        if (ssid.length() == 0) {
            Log(ERROR, "WiFi: STA mode but no SSID configured — falling back to AP\r\n");
            startAP(false);
            return 1;
        }

        WiFi.disconnect(true, true);
        delay(1000);
        WiFi.mode(WIFI_OFF);
        delay(1000);
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(GetHostName().c_str());
        SetLEDStatus(WIFI_CONNECTING, 250);

        Log(LOG, "WiFi: connecting to %s (30s timeout)\r\n", ssid.c_str());
        DisplayWiFiConnect();
        WiFi.begin(ssid.c_str(), pass.c_str());

        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED) {
            LEDUpdate();
            delay(20);
            if (millis() - t > WIFI_STA_TIMEOUT_MS) break;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Log(LOG, "WiFi: connected — IP %s\r\n", WiFi.localIP().toString().c_str());
            if (MDNS.begin(GetHostName().c_str())) {
                MDNS.addService("http", "tcp", 80);
                Log(LOG, "WiFi: mDNS — http://%s.local\r\n", GetHostName().c_str());
            } else {
                Log(ERROR, "WiFi: mDNS failed\r\n");
            }
            WiFiOK();
            return 1;
        }

        Log(ERROR, "WiFi: STA timed out after 30s — switching to AP recovery mode\r\n");
        DisplayLog("WiFi failed — AP mode");
        startAP(true);
        return 1;
    }

    startAP(false);
    return 1;
}

void WiFiRecoveryLoop(void) {
    if (!_inAPRecovery) return;
    /* Reset timeout while a client is connected so config session isn't interrupted */
    if (WiFi.softAPgetStationNum() > 0) {
        _apRecoveryStart = millis();
        return;
    }
    if (millis() - _apRecoveryStart > WIFI_AP_RECOVERY_MS) {
        Log(NOTIFY, "WiFi: AP recovery timeout — no clients, restarting to retry STA\r\n");
        delay(100);
        ESP.restart();
    }
}

static void startAP(bool recovery) {
    _inAPRecovery    = recovery;
    _apRecoveryStart = millis();
    SetMQTTLockout(true);
    String apSsid = GetHostName();
    Log(LOG, "WiFi: AP mode — SSID=%s\r\n", apSsid.c_str());
    Log(NOTIFY, "WiFi: AP password — %s\r\n", GetAPPassword().c_str());
    WiFi.disconnect(true);
    delay(250);
    WiFi.mode(WIFI_AP);
    delay(250);
    WiFi.setHostname(apSsid.c_str());
    String apPass = GetAPPassword();
    WiFi.softAP(apSsid.c_str(), apPass.c_str());
    Log(LOG, "WiFi: AP IP %s\r\n", WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(apSsid.c_str())) {
        MDNS.addService("http", "tcp", 80);
        Log(LOG, "WiFi: mDNS — http://%s.local\r\n", apSsid.c_str());
    } else {
        Log(ERROR, "WiFi: mDNS failed\r\n");
    }
    DisplaySetAPMode(true, apSsid.c_str());
    DisplayBrightnes(25);
    DisplayAPInfo(apSsid.c_str());
}

char GetWiFisetupMode(void) {
    return (char)GetWiFiMode();
}

void SetWiFisetupMode(char value) {
    (void)value;
}

static String IpAddress2String(const IPAddress& ipAddress) {
    return String(ipAddress[0]) + "." + String(ipAddress[1]) + "." +
           String(ipAddress[2]) + "." + String(ipAddress[3]);
}

String GetIPStr() {
    if (GetWiFiMode() == WIFI_AP_MODE)
        return WiFi.softAPIP().toString();
    return WiFi.localIP().toString();
}

String GetRSSIStr() {
    return String(WiFi.RSSI());
}

String GetMACStr() {
    byte mac[6];
    WiFi.macAddress(mac);
    return String(mac[5]) + ":" + String(mac[4]) + ":" + String(mac[3]) + ":" +
           String(mac[2]) + ":" + String(mac[1]) + ":" + String(mac[0]);
}

String GetAPPassword() {
    byte mac[6];
    WiFi.macAddress(mac);
    char buf[7];
    snprintf(buf, sizeof(buf), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(buf);
}
