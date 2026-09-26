#include "DeviceConfig.h"
#include <Arduino.h>

static int DeviceType = 0;

int GetDeviceType(){
  return DeviceType;
}

char QueryLocalDevice(){
  // Placeholder until board ID detection exists — all current boards are type 1.
  DeviceType = 1;
  return 1;
}

String GetBoardName(){
  return String();
}
