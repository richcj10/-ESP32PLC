#ifndef UIPAGES_H
#define UIPAGES_H

#include <stdint.h>

void     UIPageInit();
void     UIPageDraw();
void     UIPageNext();
void     UIPagePrev();
void     UIPageSelect();
uint16_t UIPageUpdateMs();
bool     UIPageLockNav();
void     UIPageTestPattern();

/* AP mode exit — hold SELECT 3 s to switch to STA and restart */
void UIApModeExitNow();

/* Execute a held-SELECT action (e.g. WiFi switch confirm) */
void UIPageExecuteHeld();

/* Upload status screen — call from main loop only */
void UIPageUpload(const char* title, uint8_t pct, const char* msg);
void UIPageUploadDone(bool success, const char* msg);

#endif
