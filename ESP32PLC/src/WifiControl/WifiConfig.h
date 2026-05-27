#ifndef WIFICONFIG_H
#define  WIFICONFIG_H

#define WIFI_STA_MODE 1
#define WIFI_AP_MODE 2

char SetupWiFi(void);
void WiFiRecoveryLoop(void);
char GetWiFisetupMode(void);
void SetWiFisetupMode(char value);
String GetIPStr();
String GetRSSIStr();
String GetMACStr();
String GetAPPassword();
#endif