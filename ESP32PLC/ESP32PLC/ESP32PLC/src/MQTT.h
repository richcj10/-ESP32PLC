#ifndef MQTT_H
#define  MQTT_H

void MqttLoop(void);
void MQTTreconnect(void);
void MQTTStart(void);
void SendChestFreezer(void);
void SendDeviceEnviroment(void);
void SendChestPower(char Mode);
void MQTTMessageInit(void);
char GetMQTTStatus(void);

#endif  /* OLED_H */
