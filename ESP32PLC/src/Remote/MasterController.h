#ifndef MASTERCONTROLLER_H
#define  MASTERCONTROLLER_H

#define REMOTE_TEMP_RTD_1 1
#define REMOTE_TEMP_RTD_2 2
#define REMOTE_TEMP_RTD_3 3

#define OUTSIDE_TEMP_POS 5
#define OUTSIDE_HUMID_POS 6
#define MC_POS 7
#define WIND_SPEED_POS 8
#define WIND_DIR_POS 9

#define REMOTE_CS_A 13
#define REMOTE_CS_B 14
#define REMOTE_CS_C 15

char RemoteStart();
void RemoteRun();
char ReadRemoteTemp();
int ReadDeviceType(int Address);
float readDeviceVIN(int Address);
char SetOcupyLED(int Address,unsigned char R,unsigned char G, unsigned char B);
char GetRemoteTemp(char TempCH);
char ReadRemoteWeather();
float GetRemoteDataFromQue(unsigned char x, bool Divide);
char ReadRemoteCurrent();

#endif