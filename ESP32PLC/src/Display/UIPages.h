#ifndef UIPAGES_H
#define UIPAGES_H

#include <stdint.h>

void UIPageInit();
void UIPageDraw();
void UIPageNext();
void UIPagePrev();
void UIPageTestPattern();

/* Upload status screen — call from main loop only */
void UIPageUpload(const char* title, uint8_t pct, const char* msg);
void UIPageUploadDone(bool success, const char* msg);

#endif
