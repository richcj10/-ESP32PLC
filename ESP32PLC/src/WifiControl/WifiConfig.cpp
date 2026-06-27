#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_netif.h>
#include "WifiControl/WifiConfig.h"
#include "FileSystem/FSInterface.h"
#include "Define.h"
#include "Functions.h"
#include "Devices/StatusLED.h"
#include "Devices/Log.h"
#include "Display/Display.h"
#include "MQTT.h"

static String IpAddress2String(const IPAddress& ipAddress);

/* Force hostname onto the lwIP STA netif — Arduino setHostname() gets
   overwritten by the framework default during WiFi.begin(). */
static void applyHostnameToNetif(const char* hostname) {
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) esp_netif_set_hostname(netif, hostname);
}

/* Returns a hostname safe for the ESP32 WiFi stack (RFC 952 / RFC 1123):
   only [a-zA-Z0-9-], max 32 chars, no leading/trailing hyphens. */
static String sanitizeHostname(const String& raw) {  // keep static — internal use only
    String out;
    out.reserve(32);
    for (size_t i = 0; i < raw.length() && out.length() < 32; i++) {
        char c = raw[i];
        if (isAlphaNumeric(c)) {
            out += (char)tolower(c);
        } else if (c == ' ' || c == '_' || c == '-') {
            out += '-';
        }
        // all other chars are dropped
    }
    // strip leading/trailing hyphens
    while (out.length() > 0 && out[0] == '-')             out.remove(0, 1);
    while (out.length() > 0 && out[out.length()-1] == '-') out.remove(out.length()-1);
    if (out.length() == 0) out = "espplc";
    return out;
}

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

        String hostname = sanitizeHostname(GetHostName());
        WiFi.disconnect(true, true);
        delay(1000);
        WiFi.mode(WIFI_OFF);
        delay(1000);
        WiFi.mode(WIFI_STA);
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(hostname.c_str());
        applyHostnameToNetif(hostname.c_str());  // must be before begin() so DHCP sends it
        SetLEDStatus(WIFI_CONNECTING, 250);

        Log(LOG, "WiFi: connecting to %s hostname=%s\r\n", ssid.c_str(), hostname.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());

        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED) {
            LEDUpdate();
            delay(20);
            if (millis() - t > WIFI_STA_TIMEOUT_MS) break;
        }

        if (WiFi.status() == WL_CONNECTED) {
            applyHostnameToNetif(hostname.c_str());
            Log(LOG, "WiFi: connected — IP %s hostname=%s\r\n", WiFi.localIP().toString().c_str(), hostname.c_str());
            if (MDNS.begin(hostname.c_str())) {
                MDNS.addService("http", "tcp", 80);
                Log(LOG, "WiFi: mDNS — http://%s.local\r\n", hostname.c_str());
            } else {
                Log(ERROR, "WiFi: mDNS failed\r\n");
            }
            WiFiOK();
            NeighborScan();
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
    String apSsid = sanitizeHostname(GetHostName());
    Log(LOG, "WiFi: AP mode — SSID=%s\r\n", apSsid.c_str());
    Log(NOTIFY, "WiFi: AP password — %s\r\n", GetAPPassword().c_str());
    WiFi.disconnect(true);
    delay(250);
    WiFi.mode(WIFI_AP);
    delay(250);
    WiFi.setHostname(apSsid.c_str());
    String apPass = GetAPPassword();
    if (!WiFi.softAP(apSsid.c_str(), apPass.c_str()))
        Log(ERROR, "WiFi: softAP() FAILED — SSID=%s pass_len=%u\r\n",
            apSsid.c_str(), (unsigned)apPass.length());
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

// ── Neighbor discovery ────────────────────────────────────────────────────────
static NeighborEntry _neighbors[NEIGHBOR_MAX];
static int           _neighborCount = 0;

void NeighborScan() {
    _neighborCount = 0;
    String selfHost = sanitizeHostname(GetHostName());
    int n = MDNS.queryService("http", "tcp");
    for (int i = 0; i < n && _neighborCount < NEIGHBOR_MAX; i++) {
        String h = MDNS.hostname(i);
        String hLow = h; hLow.toLowerCase();
        if (hLow == selfHost) continue;
        if (!hLow.startsWith("espplc")) continue;
        strlcpy(_neighbors[_neighborCount].name, h.c_str(),                    48);
        strlcpy(_neighbors[_neighborCount].ip,   MDNS.IP(i).toString().c_str(), 16);
        _neighborCount++;
    }
    Log(LOG, "Neighbors: found %d\r\n", _neighborCount);
}

int NeighborCount() { return _neighborCount; }
const NeighborEntry* NeighborGet(int i) {
    if (i < 0 || i >= _neighborCount) return nullptr;
    return &_neighbors[i];
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

String GetSanitizedHostname() { return sanitizeHostname(GetHostName()); }

String GetAPPassword() {
    byte mac[6];
    WiFi.macAddress(mac);
    char buf[9];  // 8 hex chars — WPA2 minimum is 8
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}
