#include "I2C.h"

char SCLPin = 0;
char SDAPin = 0;

void SetI2CWires(char Type, char Input){
    if (Type == SCLPIN)
    {
        SDAPin = Input;
    }
    if (Type == SCLPIN)
    {
        SCLPin = Input;
    }
}
void I2CStart(){

}

void I2CRun(){

}