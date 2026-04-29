#ifndef PLCMOD_WEATHERSTATION_H
#define PLCMOD_WEATHERSTATION_H

#include "Remote/RemoteConfig.h"

// WeatherStation module descriptor — typeId=2
// 7-register input block: device type, temp, humidity, moisture,
// wind speed, wind direction (raw deg), rain.

#pragma message("PLCMod enabled: WeatherStation  (typeId=2  swVersion=0x0100)")

extern const PLCModuleDesc_t WeatherStation_Desc;

#endif // PLCMOD_WEATHERSTATION_H
