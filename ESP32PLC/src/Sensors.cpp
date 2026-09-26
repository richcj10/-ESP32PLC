#include "Sensors.h"
#include "Devices/TempHumid.h"

static float DeviceTempratureF = 0;
static float DeviceHumidity    = 0;

void InitSensors(void){
  climSensor.begin();
}

void UpdateSensors(void){
  climSensor.update();
  DeviceHumidity    = climSensor.getHumidity();
  DeviceTempratureF = climSensor.getTempF();
}

float getDeviceClimateHumidity(){
  return DeviceHumidity;
}

float getDeviceClimateTemprature(){
  return DeviceTempratureF;
}
