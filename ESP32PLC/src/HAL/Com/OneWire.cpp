#include "OneWire.h"

char OneWireIOPin = 0;

void SetOneWireIO(char IOValue){
    OneWireIOPin = IOValue;
}