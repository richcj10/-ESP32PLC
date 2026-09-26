#ifndef MQTT_H
#define  MQTT_H

#include <PubSubClient.h>

void MqttLoop(void);
void MQTTreconnect(void);
void MQTTStart(void);
PubSubClient& GetMQTTClient(void);
char        GetMQTTStatus(void);
const char* GetMQTTState(void);      // "disabled" | "connected" | "connecting" | "ap_mode" | "failed"
const char* GetMQTTBaseTopic(void);  // "ESPPLC/<hostname>"
void SendRemoteDevices();   // publishes all JSON-configured devices per group mqttTopic
void PublishHADiscovery();  // HA MQTT auto-discovery config topics (called on connect)
void SendLocalIO();         // publishes current input states to ESPPLC/<host>/io/in/<n>
void SetMQTTLockout(char Mode);

#endif  /* OLED_H */
