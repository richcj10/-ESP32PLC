#ifndef STATUSLED_H
#define  STATUSLED_H

/////#define PixelCount 4 // this example assumes 4 pixels, making it smaller will cause a failure
//#define PixelPin 2

#define colorSaturation 128


void StatusLEDStart();
void SetStatus();
char GetStatus();

#endif 