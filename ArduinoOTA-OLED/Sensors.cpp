#include "Sensors.h"
#include "SparkFun_Si7021_Breakout_Library.h"
#include <OneWire.h>

float DeviceHumidity = 0;
float DeviceTempratureF = 0;

Weather sensor;

void InitSensors(void){
  sensor.begin();
  InitOneWire();
}

void InitOneWire(void){
  
}

void readDeviceClimate()
{
  // Measure Relative Humidity/Temp from the HTU21D or Si7021
  DeviceHumidity = sensor.getRH();
  DeviceTempratureF = sensor.getTempF();
}

void printInfo(){
//This function prints the weather data out to the default Serial Port

  Serial.print("Temp:");
  Serial.print(DeviceTempratureF);
  Serial.print("F, ");

  Serial.print("Humidity:");
  Serial.print(DeviceHumidity);
  Serial.println("%");
}

float getDeviceClimateHumidity(){
  return DeviceHumidity;
}

float getDeviceClimateTemprature(){
  return DeviceTempratureF;
}
