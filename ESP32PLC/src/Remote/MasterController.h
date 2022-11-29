#ifndef MASTERCONTROLLER_H
#define  MASTERCONTROLLER_H

char RemoteStart();
void RemoteRun();
int ReadDeviceType(int Address);
float readDeviceVIN(int Address);
char SetOcupyLED(int Address,unsigned char R,unsigned char G, unsigned char B);

#endif