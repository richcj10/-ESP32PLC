#include "DeviceConfig.h"

int DeviceType = 0;

int GetDeviceType(){
  return DeviceType;
}

void SetDeviceType(int value){
  DeviceType = value;
}

void QueryLocalDevice(){
  //Find the local device using board ID, this will most likly be one wire device. 
  DeviceType = 1; //Place holder till code is done. 
}
