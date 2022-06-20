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

void DislaySaver();
void DisplayTimeoutReset();
void DisplayManager();
void DisplayUserHandeler(void);
void DisplaySwitchCase();
void DispalySleepControl(char value);
void DisplaySetup();
void DispalyConfigSet(char value);
void DisplayWiFiSignal();
void DisplayLogo();
void DisplayBrightnes(char Brightness);
void DisplayWiFiConnect();
void DisplayLog(const char *Text);


#endif
