#include "I2C.h"
#include <Arduino.h>
#include <Wire.h>

char SCLPin = 15;
char SDAPin = 7;

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
    Wire.begin(SDAPin, SCLPin);
}

void I2CRun(){

}

void I2CScan(){
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }   
}