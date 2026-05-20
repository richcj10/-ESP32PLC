#ifndef MQTT_H
#define  MQTT_H

#include <PubSubClient.h>

void MqttLoop(void);
void MQTTreconnect(void);
void MQTTStart(void);
PubSubClient& GetMQTTClient(void);
//void SendChestFreezer(void);
void SendDeviceEnviroment(void);
//void SendChestPower(char Mode);
//void MQTTMessageInit(void);
char        GetMQTTStatus(void);
const char* GetMQTTBaseTopic(void);  // "ESPPLC/<hostname>"
void SendRemoteDevices();   // publishes all JSON-configured devices per group mqttTopic
void SetMQTTLockout(char Mode);

#endif  /* OLED_H */
