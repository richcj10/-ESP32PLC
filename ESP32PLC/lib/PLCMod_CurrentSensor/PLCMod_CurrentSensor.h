#ifndef PLCMOD_CURRENTSENSOR_H
#define PLCMOD_CURRENTSENSOR_H

#include "Remote/RemoteConfig.h"

// CurrentSensor module descriptor — typeId=3
// 3-phase AC current measurement. Channels A/B/C, /100 = Amps.

#pragma message("PLCMod enabled: CurrentSensor  (typeId=3  swVersion=0x0100)")

extern const PLCModuleDesc_t CurrentSensor_Desc;

#endif // PLCMOD_CURRENTSENSOR_H
