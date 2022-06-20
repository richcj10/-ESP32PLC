#ifndef TFT_H
#define  TFT_H

#define TFTWIFIBITMAPX 40
#define TFTWIFIBITMAPY 8

#define TFTSCREEN_WIDTH 256 // OLED display width, in pixels
#define TFTSCREEN_HEIGHT 180 // OLED display height, in pixels

#define TFTMQTTCX 20
#define TFTMQTTCY 0

#define CENTERYTOP 20
#define CENTERYBOT 55
#define TFTBANNERY 160

#define INFOY 175
#define INFOX 90

void TFTInit();
void TFTWiFiSignal(char overide);
void TFTLogoDisplay();
void TFTWiFiConnect(char Position);
void TFTTHBar();
void TFTLog(const char *Comment);

void TFTBarClear();
void TFTLogo();


#endif