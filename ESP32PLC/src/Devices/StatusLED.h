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
void SetLEDStatus(char mode, int rate);
char GetStatus();
void LEDBoot();
void LEDUpdate();
#include <stdint.h>
void StatusLEDSet(uint8_t r, uint8_t g, uint8_t b);  // status pixel only
void StatusLEDOverride(bool on);   // true = LEDUpdate() leaves the LED alone

void WiFiFcn();
void MQTTFcn();
void NormalFcn();

#endif 