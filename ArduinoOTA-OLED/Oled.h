#ifndef OLED_H
#define  OLED_H

void DisplayInit(void);
void Bitmap16xBitMapClear(char Location);
void WiFiConnectAnimation(void);
void WiFiStreanthDisplay(char power);
void MQTTIconSet(char IconMode);
void WiFiAP(char Enable);
void DisplayTHBar();
void FullDisplayClear(void);
void DisplayCenterClear(void);
void DisplayCenterChestTemp(void);
void THBarClear();
void CenterClear();

#endif  /* OLED_H */
