#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <stdint.h>

char FileSystemInit(struct WiFiConfig* WFC, struct MQTTConfig* MQC);
void WifisaveConfiguration(struct WiFiConfig* WFC);
bool WifiloadConfiguration(struct WiFiConfig* WFC);   // false = file absent
void MqttsaveConfiguration(struct MQTTConfig* MQC);
bool MqttloadConfiguration(struct MQTTConfig* MQC);   // false = file absent
void PrintWiFiConfigStruct(struct WiFiConfig* WFC);
void PrintMqttConfigStruct(struct MQTTConfig* MQC);
void WifiComfig(struct WiFiConfig* WFC);
void MqttComfig(struct MQTTConfig* MQC);
void RemoteComfig();
void SaveHostName(struct WiFiConfig* WFC);

struct WiFiConfig {
    unsigned char WIFIMode        = 2;          // 1=STA  2=AP (default AP)
    char SSID[32]                 = "";          // empty until user configures
    unsigned char SSIDLN          = 0;
    char Passcode[40]             = "";
    unsigned char PswdLN          = 0;
    char Host[40]                 = "ESP32PLC";
    unsigned char HoastLN         = 8;
    unsigned char DHCP            = 1;
    char IP[16]                   = "192.168.4.1";
    char DefultGateway[16]        = "0.0.0.0";
    char SubMask[16]              = "255.255.255.0";
};

struct MQTTConfig {
    unsigned char MQTTEnabble     = 0;           // disabled until user configures
    char MQTTIP[40]               = "";
    char MQTTUser[40]             = "esp32plc";
    char MQTTPassword[40]         = "";
    unsigned char MQTTPasswordLN  = 0;
    uint16_t MQTTPort             = 1883;
};

#endif
