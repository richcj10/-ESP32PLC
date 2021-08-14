#ifndef DEVICECONFIGPVT_H
#define  DEVICECONFIGPVT_H

#include "Define.h"
#include <WString.h>
#include "DeviceConfig.h"

char IOArray[] = {4,5,12,13,14,15,16,17,18,19,21,11,23,25,26,27,32,34,35,36,39};

typedef struct {
   String name;
   char IO4;
   char IO5;
   char IO12;
   char IO13;
   char IO14;
   char IO15;
   char IO16;
   char IO17;
   char IO18;
   char IO19;
   char IO21;
   char IO22;
   char IO23;
   char IO25;
   char IO26;
   char IO27;
   char IO32;
   char IO34;
   char IO35;
   char IO36;
   char IO39;
}ESP32PLCIO;

typedef struct{
    ESP32PLCIO ESP32PLCIOCountInvent[HOW_MANY_IO_TYPES];
} inventory;

inventory inv = {
    {
        {"Defult",0,ONEWIRE,0,IOOUTPUT,IOINPUT,0,UART1TX,UART1RX,IOOUTPUT,NEOPIXEL,IOSDA,IOSCL,0,0,IOINPUT,IOOUTPUT,0,IOOUTPUT,IOOUTPUT,IOOUTPUT,1},  /* init SwordInvent[0] */
        {"Inputs",1,ONEWIRE,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},  /* init SwordInvent[1] */
        {"Outputs",1,ONEWIRE,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},  /* init SwordInvent[1] */
        {"Nepixel",1,ONEWIRE,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},  /* init SwordInvent[1] */
    }
};


/*
#define UNUSED 0
#define INPUT 1
#define OUTPUT 2
#define UART1TX 3
#define UART1RX 4
#define SCL 5
#define SDA 6
#define NEOPIXEL 7
#define ONEWIRE 8

#define LED 2
#define USER_SW 0
#define MP1INPUT 26
#define MP2INPUT 14
//#define MP3INPUT 2
#define MP4INPUT 3

#define MP1OUT 34
#define MP2OUT 12
#define MP3OUT 39
#define MP4OUT 27
#define MP5OUT 36
#define MP6OUT 35

*/


#endif