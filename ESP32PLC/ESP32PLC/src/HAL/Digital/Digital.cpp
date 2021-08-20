#include "Digital.h"
#include <Arduino.h>

int mode = 0;


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
        Serial.println("Invalid Config");
        break;
    }
}