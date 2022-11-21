#include "JoyStick.h"
#include <Arduino.h>
#include "Devices/Log.h"

int DataWindow[3] = {0,0,0};
int Average = 0;
char ButtonSel = 0;
char ButtonSelStatus = 0;

long JoystickRefreshRate = 0;
unsigned long JoystickcurrentMillis = 0;
unsigned long lastTimeButtonSel = 0;

void JoyStickStart(){
    pinMode(1,INPUT);
    pinMode(0,INPUT);
    //analogReadResolution(12);
    DataWindow[0] = analogRead(1);
    DataWindow[1] = analogRead(1);
    DataWindow[2] = analogRead(1);
}

char GetJoyStickPos(){
    //Serial.print("JoyStick = ");
    //Serial.println(Average);
    char JoystickState = 0;
    switch (Average){
        case 3800 ... 4095:
            JoystickState = JOYSTICK_NONE;
            break;
        case 550 ... 780:
            JoystickState = JOYSTICK_UP;
            break;
        case 400 ... 520:
            JoystickState = JOYSTICK_DOWN;
            break;
        case 800 ... 1100:
            JoystickState = JOYSTICK_LEFT;
            break;
        case 1120 ... 1400:
            JoystickState = JOYSTICK_RIGHT;
            break;
        default:
            JoystickState = JOYSTICK_ERROR;
            break;
        }
    return JoystickState;
}

char r = 0;

void JoyStickUpdate(){
    JoystickcurrentMillis = millis();
    char readingButtonSel = digitalRead(0);
    if (readingButtonSel != ButtonSel) {
         lastTimeButtonSel = JoystickcurrentMillis;
    }
    if((JoystickcurrentMillis - lastTimeButtonSel) > 50){
        if (readingButtonSel != ButtonSelStatus) {
            ButtonSelStatus = readingButtonSel;
            Log(DEBUG,"Joystick BTN\r\n");
        }
    }
    if (JoystickcurrentMillis - JoystickRefreshRate >= 80) {
        JoystickRefreshRate = JoystickcurrentMillis;
        Log(DEBUG,"Joystick ANG READ\r\n");
         long AvargeCalc = 0;
         DataWindow[r] = analogRead(1);
         r++;
         if(r > 3){
            r = 0;
         }
         AvargeCalc = DataWindow[0] + DataWindow[1] + DataWindow[2];
         Average = AvargeCalc/3;
    }
}

void GetJoystickPrint(char x){
    switch (x){
        case JOYSTICK_NONE:
            break;
        case JOYSTICK_UP:
            Serial.println("Up");
            break;
        case JOYSTICK_DOWN:
            Serial.println("Down");
            break;
        case JOYSTICK_LEFT:
            Serial.println("Left");
            break;
        case JOYSTICK_RIGHT:
            Serial.println("Right");
            break;
        default:
            //Serial.println("Error");
            break;
        }    
}

char GetJoyStickSelect(){
    //return ButtonSelStatus;
    return !digitalRead(0);
}