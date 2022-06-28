#ifndef TFT_H
#define  TFT_H

#define TFTWIFIBITMAPX 40
#define TFTWIFIBITMAPY 8

#define TFTMQTTCX 65
#define TFTMQTTCY 8

#define TFTSCREEN_WIDTH 256 // OLED display width, in pixels
#define TFTSCREEN_HEIGHT 180 // OLED display height, in pixels

#define CENTERYTOP 20
#define CENTERYBOT 55
#define TFTBANNERY 200

#define INFOY 165
#define INFOX 80

void TFTInit();
void TFTWiFiSignal(char overide);
void TFTLogoDisplay();
void TFTWiFiConnect(char Position);
void TFTTHBar();
void TFTLog(const char *Comment);
void TFTDisplayClear();

void TFTMQTTIconSet(char IconMode);

void TFTBarClear();
void TFTLogo();


#endif