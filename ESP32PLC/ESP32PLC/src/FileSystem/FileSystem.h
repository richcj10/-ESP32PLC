#ifndef FILESYSTEM_H
#define  FILESYSTEM_H

char FileSystemInit(struct WiFiConfig* WFC,struct MQTTConfig* MQC);
void WifisaveConfiguration(struct WiFiConfig* WFC);
void WifiloadConfiguration(struct WiFiConfig* WFC);
void MqttsaveConfiguration(struct MQTTConfig* MQC);
void MqttloadConfiguration(struct MQTTConfig* MQC);
void PrintWiFiConfigStruct(struct WiFiConfig* WFC);
void PrintMqttConfigStruct(struct MQTTConfig* MQC);
void WifiComfig(struct WiFiConfig* WFC);
void MqttComfig(struct MQTTConfig* MQC);
void SaveHostName(struct WiFiConfig* WFC);

struct WiFiConfig {
  char SSID[32] = "Lights.Camera.Action";
  unsigned char SSIDLN = 20;
  char Passcode[40] = "RR58fa!8";
  unsigned char PswdLN = 8;
  char Host[40] ="ESP32PLC-1234";
  unsigned char HoastLN = 13;
  unsigned char DHCP = 1;
  char IP[16] = "192.168.5.10";
  char DefultGateway[16] = "000.000.000.000";
  char SubMask[16] = "000.000.000.000";
};

struct MQTTConfig {
  unsigned char MQTTEnabble = 1;
  char MQTTIP[16] = "000.000.000.000";;
  char MQTTPassword[40] = "Stuff";
  unsigned char MQTTPasswordLN = 5;
};



#endif