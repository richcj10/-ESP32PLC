#ifndef DIGITAL_H
#define  DIGITAL_H

void ScanArrayAdd(char IOpin);
void DigitalStart(void);
void SetUserLED(bool Value);
bool GetUserSWValue(void);
void ToggletUserLED(void);
void ScanUserInput(void);

#endif