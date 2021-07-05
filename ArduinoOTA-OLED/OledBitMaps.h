#ifndef OLEDBITMAPS_H
#define  OLEDBITMAPS_H
#include <pgmspace.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define WIFIBITMAPX 0
#define WIFIBITMAPY 0

#define MQTTCX 20
#define MQTTCY 0

const unsigned char PROGMEM Home[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x03, 0xC0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x1F, 0xF8, 0x00,
0x00, 0x3F, 0xFC, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x80,
0x03, 0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF8,
0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xF8, 0xF0, 0x0F, 0x03, 0xC0, 0xF0,
0x0F, 0x03, 0xC0, 0xF0, 0x0F, 0x03, 0xC0, 0xF0, 0x0F, 0x03, 0xC0, 0xF0, 0x0F, 0x03, 0xC0, 0xF0,
0x0F, 0x03, 0xC0, 0xF0, 0x0F, 0x03, 0xFF, 0xF0, 0x0F, 0x03, 0xFF, 0xF0, 0x06, 0x01, 0xFF, 0xE0,
0x02, 0x01, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM Sig [] = {
0x00, 0x00, 0x10, 0x08, 0x30, 0x0C, 0x64, 0x26, 0x4C, 0x32, 0xDA, 0x5B, 0x96, 0x69, 0xB5, 0xAD,
0xB5, 0xAD, 0x96, 0x69, 0xDA, 0x5B, 0x4C, 0x32, 0x64, 0x26, 0x30, 0x0C, 0x10, 0x08, 0x00, 0x00
};

const unsigned char PROGMEM SignalMed [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x0C, 0x30, 0x1A, 0x58, 0x16, 0x68, 0x35, 0xAC,
0x35, 0xAC, 0x16, 0x68, 0x1A, 0x58, 0x0C, 0x30, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


const unsigned char PROGMEM SignalSmall [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x40, 0x06, 0x60, 0x05, 0xA0,
0x05, 0xA0, 0x06, 0x60, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM NotConnected[] = {
0x03, 0xC0, 0x1F, 0xF8, 0x3E, 0x7C, 0x78, 0x1E, 0x74, 0x2E, 0x6E, 0x76, 0xE7, 0xE7, 0xC3, 0xC3,
0xC3, 0xC3, 0xE7, 0xE7, 0x6E, 0x76, 0x74, 0x2E, 0x78, 0x1E, 0x3E, 0x7C, 0x1F, 0xF8, 0x03, 0xC0
};

const unsigned char PROGMEM Connected[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x08, 0xC0, 0x10, 0x70, 0x10, 0x08, 0x30, 0x0C,
0x42, 0x42, 0x44, 0x22, 0x44, 0xA2, 0x45, 0x26, 0x36, 0x68, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM Clear[] = {
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

#endif  /* OLED_H */