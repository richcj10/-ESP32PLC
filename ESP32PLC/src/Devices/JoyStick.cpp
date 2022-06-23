#include "JoyStick.h"
#include <Arduino.h>

int DataWindow[3] = {0,0,0};
int Average = 0;
char ButtonSel = 0;
char ButtonSelStatus = 0;

long JoystickRefreshRate = 0;
unsigned long JoystickcurrentMillis = 0;
unsigned long lastTimeButtonSel = 0;

void JoyStickStart(){
    pinMode(1,INPUT);
    //analogReadResolution(12);
    for(int i = 0;i<6;i++){
        DataWindow[i] = analogRead(1);
        //Serial.println(DataWindow[i]);
    }
}

char GetJoyStickPos(){
    Serial.print("JoyStick = ");
    Serial.println(Average);
    char JoystickState = 0;
    switch (Average){
        case 3800 ... 4095:
            JoystickState = JOYSTICK_NONE;
            break;
        case 1050 ... 1250:
            JoystickState = JOYSTICK_UP;
            break;
        case 1450 ... 1650:
            JoystickState = JOYSTICK_DOWN;
            break;
        case 550 ... 650:
            JoystickState = JOYSTICK_LEFT;
            break;
        case 1850 ... 1950:
            JoystickState = JOYSTICK_RIGHT;
            break;
        default:
            JoystickState = JOYSTICK_ERROR;
            break;
        }
    return JoystickState;
}

void JoyStickUpdate(){
    JoystickcurrentMillis = millis();
    char readingButtonSel = digitalRead(0);
    if (readingButtonSel != ButtonSel) {
        lastTimeButtonSel = JoystickcurrentMillis;
    }
    if((millis() - lastTimeButtonSel) > 50){
        if (readingButtonSel != ButtonSelStatus) {
            ButtonSelStatus = readingButtonSel;
        }
    }
    if (JoystickcurrentMillis - JoystickRefreshRate >= 20) {
        JoystickRefreshRate = JoystickcurrentMillis;
        long AvargeCalc = 0;
        DataWindow[0] = DataWindow[1];
        DataWindow[1] = DataWindow[2];
        DataWindow[2] = analogRead(1);
        AvargeCalc = DataWindow[0] + DataWindow[1] + DataWindow[2];
        Average = AvargeCalc/3;
    }
}

char GetJoyStickSelect(){
    //return ButtonSelStatus;
    return !digitalRead(0);
}