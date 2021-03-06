#ifndef MENU_H
#define  MENU_H

#define TFT 1
#define OLED 2

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT  12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define LED_PIN            9

void DisplaySaver();
void DisplayTimeoutReset();
void DisplayManager();
void DisplaySwitchCase();
void DispalySleepControl(char value);
void DisplaySetup();
void DispalyConfigSet(char value);
void DisplayWiFiSignal();
void DisplayLogo();
void DisplayBrightnes(char Brightness);
void DisplayWiFiConnect();
void DisplayLog(const char *Text);
void DisplayClear();
void WiFiCheckRSSI(char updateDisp);
void DisplayMQTT(char Mode);
void DisplayTHBar();
void DisplayID();
void DisplayCenterInput(void);
void DisplayCenterOutput(void);
void DisplayCenterIPInfo(void);
void DisplayCenterRemoteInfo(void);

#endif
