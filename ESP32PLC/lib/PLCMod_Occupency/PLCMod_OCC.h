#ifndef PLCMOD_OCC_H
#define PLCMOD_OCC_H

#include "Remote/RemoteConfig.h"

// OCC-TOL module descriptor — typeId=2  swVersion=0x0100
// Occupancy / Time-on-Light sensor.
// Registers: OccStatus, ZoneCount, TimeOnSec  (FC4, startReg=0, count=3)
// LED control: FC16 writes via SetOcupyLED() in MasterController.

#pragma message("PLCMod enabled: OCC-TOL  (typeId=2  swVersion=0x0100)")

extern const PLCModuleDesc_t OCC_Desc;

#endif // PLCMOD_OCC_H
