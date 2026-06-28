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
String GetSanitizedHostname();

#define NEIGHBOR_MAX 8
struct NeighborEntry { char name[48]; char ip[16]; };

void                 NeighborScan();
int                  NeighborCount();
const NeighborEntry* NeighborGet(int i);

#endif