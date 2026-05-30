#include "RS485.h"

char UARTRXPin = 0;
char UARTTXPin = 0;

void SetUARTWires(char Type, char Input){
    if (Type == RX)
    {
        UARTRXPin = Input;
    }
    if (Type == TX)
    {
        UARTTXPin = Input;
    }
}