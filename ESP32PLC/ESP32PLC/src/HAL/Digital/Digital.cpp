#include "Digital.h"
#include <Arduino.h>
#include "Define.h"

int mode = 0;

bool LastUserSWValue = 0;
bool UserSWValue = 0;
bool UserLEDValue = 0;
unsigned long lastDebounceTime = 0;

char InputScanArray[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool InputScanLastValue[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool InputScanCurrentValue[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned char InputScanCount = 0;

void ScanIO(void){
    switch (mode){
    case 1:
        /* code */
        break;
    case 2:
        /* code */
        break;
    case 3:
        /* code */
        break;   
    default:
        Serial.print("Invalid Config");
        break;
    }
}

void DigitalStart(void){
    pinMode(LED,OUTPUT);
    pinMode(USER_SW,INPUT);
}

void SetUserLED(bool Value){
    UserLEDValue = Value;
    digitalWrite(LED,UserLEDValue);
}

void ToggletUserLED(){
    UserLEDValue = !UserLEDValue;
    digitalWrite(LED,UserLEDValue);
}

void ScanUserInput(void){
    char Value = digitalRead(USER_SW);
    if(LastUserSWValue != Value){
        lastDebounceTime = millis();
    }
    if((millis() - lastDebounceTime) > 200){
        UserSWValue = Value;
    }
    LastUserSWValue = Value;
}

bool GetUserSWValue(void){
    return !UserSWValue;
}

void ScanArrayAdd(char IOpin){
    InputScanCount++;
    InputScanArray[InputScanCount] = IOpin;
}