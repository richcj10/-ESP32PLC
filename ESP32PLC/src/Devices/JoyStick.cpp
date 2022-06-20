#include "JoyStick.h"
#include <Arduino.h>

int DataWindow[6] = {0,0,0,0,0,0};
int Average = 0;


long JoystickRefreshRate = 0;
unsigned long JoystickcurrentMillis = 0;

void JoyStickStart(){
    pinMode(1,INPUT);
    analogReadResolution(12);
    for(int i = 0;i<6;i++){
        DataWindow[i] = analogRead(1);
        //Serial.println(DataWindow[i]);
    }
}

char GetJoyStickPos(){
    //Serial.print("JoyStick = ");
    //Serial.println(Avarge);
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
  if (JoystickcurrentMillis - JoystickRefreshRate >= 100) {
      JoystickRefreshRate = JoystickcurrentMillis;
/*     long AvargeCalc = 0;
    for(int i = 0;i<4;i++){
        DataWindow[i] = DataWindow[i+1];
        //Serial.println(DataWindow[i]);
    }
    DataWindow[5] = analogRead(1); */
     Average = analogRead(1);
/*     for(int i = 0;i<5;i++){
        AvargeCalc = DataWindow[i] + AvargeCalc;
    }
    Avarge = AvargeCalc/6; */
    //Serial.println(Avarge);
  }
}

char GetJoyStickSelect(){
    return 0;
}