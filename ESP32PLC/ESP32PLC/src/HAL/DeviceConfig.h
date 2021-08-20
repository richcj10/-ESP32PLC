#ifndef DEVICECONFIG_H
#define  DEVICECONFIG_H

int GetDeviceType();
void SetDeviceType(int value);
char QueryLocalDevice();
char GPIOStart();
void SetIOType(char IOValue, unsigned int IOLocation);

#define UNUSED 0
#define IOINPUT 1
#define IOOUTPUT 2
#define UART1TX 3
#define UART1RX 4
#define IOSCL 5
#define IOSDA 6
#define NEOPIXEL 7
#define ONEWIRE 8

#endif
