#ifndef OLED_H
#define  OLED_H

void OLEDInit(void);
void Bitmap16xBitMapClear(char Location);
void WiFiConnectAnimation(void);
void WiFiStreanthDisplay(char power);
void CheckMQTTCon(char overide);
void OLEDMQTTIconSet(char IconMode);
void WiFiAP(char Enable);
void OLEDTHBar(void);
void OledDisplayClear(void);
void DisplayCenterClear(void);
void DisplayCenterChestTemp(void);
void THBarClear();
void CenterClear();
void OLEDIDDisplay();
void DisplayCenterOutput(void);
void DisplayCenterInput(void);
void DisplayCenterOutputRS485(void);
void DisplayTimeToReset(long Time);
#endif  /* OLED_H */
