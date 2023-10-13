#ifndef MASTERCONTROLLER_H
#define  MASTERCONTROLLER_H

char RemoteStart();
void RemoteRun();
char ReadRemoteTemp();
int ReadDeviceType(int Address);
float readDeviceVIN(int Address);
char SetOcupyLED(int Address,unsigned char R,unsigned char G, unsigned char B);
char GetRemoteTemp(char TempCH);

#endif