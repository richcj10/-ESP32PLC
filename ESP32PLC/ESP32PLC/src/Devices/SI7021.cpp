#include "SI7021.h"
#include <Arduino.h>
#include "HAL/Com/I2C.h"

/****************Si7021 & HTU21D Functions**************************************/

//#define _BV(bit) (1 << (bit))

float ESgetRH(){
  // Measure the relative humidity
  int RH_Code = Si7021makeMeasurment(HUMD_MEASURE_NOHOLD);
  float result = (125.0*RH_Code/65536)-6;
  return result;
}

float ESreadTemp(){
  // Read temperature from previous RH measurement.
  int temp_Code = Si7021makeMeasurment(TEMP_PREV);
  float result = (175.72*temp_Code/65536)-46.85;
  return result;
}

float ESgetTemp(){
  // Measure temperature
  int temp_Code = Si7021makeMeasurment(TEMP_MEASURE_NOHOLD);
  float result = (175.72*temp_Code/65536)-46.85;
  return result;
}

//Give me temperature in fahrenheit!
float ESreadTempF(){
  return((ESreadTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

float ESgetTempF(){
  return((ESgetTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

void Si7021heaterOn(){
  // Turns on the ADDRESS heater
  //uint8_t regVal = Si7021readReg();
  //regVal |= _BV(HTRE);
  //turn on the heater
  //Si7021writeReg(regVal);
}

void Si7021heaterOff(){
  // Turns off the ADDRESS heater
  //uint8_t regVal = Si7021readReg();
  //regVal &= ~_BV(HTRE);
  //Si7021writeReg(regVal);
}

void Si7021changeResolution(char i){
  // Changes to resolution of ADDRESS measurements.
  // Set i to:
  //      RH         Temp
  // 0: 12 bit       14 bit (default)
  // 1:  8 bit       12 bit
  // 2: 10 bit       13 bit
  // 3: 11 bit       11 bit
  //uint8_t regVal = Si7021readReg();
  uint8_t regVal = 0;
  // zero resolution bits
  regVal &= 0b011111110;
  switch (i) {
    case 1:
      regVal |= 0b00000001;
      break;
    case 2:
      regVal |= 0b10000000;
      break;
    case 3:
      regVal |= 0b10000001;
    default:
      regVal |= 0b00000000;
      break;
  }
  // write new resolution settings to the register
  //Si7021writeReg(regVal);
}

void Si7021reset(){
  //Reset user resister
  //Si7021writeReg(SOFT_RESET);
}

char Si7021checkID(){
    char ID_1;
    // Check device ID
    //Wire.beginTransmission(ADDRESS);
    //Wire.write(0xFC);
    //Wire.write(0xC9);
    //Wire.endTransmission();
    //Wire.requestFrom(ADDRESS,1);
    //ID_1 = Wire.read();
    //return(ID_1);
    if(ID_1 == 0x15){
      //Serial.println("Si7021 Found");
      return 1;
    }
    else{
        return 0;
    }
}

int Si7021makeMeasurment(char command){
    char Data[3]; 
    // Take one ADDRESS measurement given by command.
    // It can be either temperature or relative humidity
    // TODO: implement checksum checking
    uint16_t nBytes = 3;
    // if we are only reading old temperature, read olny msb and lsb
    if (command == 0xE0) nBytes = 2;
    I2CwriteReg(ADDRESS, command);
    // When not using clock stretching (*_NOHOLD commands) delay here
    // is needed to wait for the measurement.
    // According to datasheet the max. conversion time is ~22ms
    delay(100);
    I2Cread(ADDRESS,nBytes, Data);
    unsigned int msb = Data[0];
    unsigned int lsb = Data[1];
    // Clear the last to bits of LSB to 00.
    // According to datasheet LSB of RH is always xxxxxx10
    lsb &= 0xFC;
    unsigned int mesurment = msb << 8 | lsb;
    return mesurment;
}

