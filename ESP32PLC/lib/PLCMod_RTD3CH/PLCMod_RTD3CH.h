#ifndef PLCMOD_RTD3CH_H
#define PLCMOD_RTD3CH_H

#include "Remote/RemoteConfig.h"

// RTD-3CH module descriptor — typeId=1
// Three RTD temperature channels + discrete switches + on-demand setpoints.
// Default reg names: RTD1/RTD2/RTD3. Override by providing a custom
// const char* const[] array at the instantiation site if needed.

#pragma message("PLCMod enabled: RTD-3CH  (typeId=1  swVersion=0x0100)")

extern const PLCModuleDesc_t RTD3CH_Desc;

#endif // PLCMOD_RTD3CH_H
