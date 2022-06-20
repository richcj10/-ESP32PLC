#ifndef STATUSLED_H
#define  STATUSLED_H

/////#define PixelCount 4 // this example assumes 4 pixels, making it smaller will cause a failure
//#define PixelPin 2

#define WIFI_CONNECTING 1
#define MQTT_CONNECTING 2
#define MQTT_SEND 3
#define RS485_TRAFIC 4
#define NORMAL 5

void StatusLEDStart();
void SetLEDStatus(char mode, char rate);
char GetStatus();
void LEDBoot();
void LEDUpdate();

void WiFiFcn();
void MQTTFcn();
void NormalFcn();

#endif 