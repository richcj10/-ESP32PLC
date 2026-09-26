#ifndef MENU_H
#define  MENU_H

#include <stdint.h>

#define TFT 1
#define OLED 2

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT  12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// TFT backlight PWM pin (LEDC channel 0, driven by _hw_brightness)
#define LED_PIN            9

void DisplaySaver();
void DisplayTimeoutReset();
void DisplayManager();
void DispalySleepControl(char value);
void DisplaySetup();
void DispalyConfigSet(char value);
void DisplayBrightnes(char Brightness);
void DisplayLog(const char *Text);
void DisplayClear();
void DisplayAPInfo(const char* ssid);
void DisplaySetAPMode(bool ap, const char* ssid);

/* Upload progress — disables screen saver and forces display on */
void DisplayUploadStatus(const char* title, uint8_t pct, const char* msg);
void DisplayUploadDone(bool success, const char* msg);

#endif
