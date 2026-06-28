#include "ModuleManager.h"

#ifdef MOD_LEDSTRIP
#include <LEDStripModule.h>
#endif
#ifdef MOD_FIREWORKS
#include <FireworksModule.h>
#endif

void RegisterModules() {
#ifdef MOD_LEDSTRIP
    modules.add(new LEDStripModule());
#endif
#ifdef MOD_FIREWORKS
    modules.add(&fwModule);
#endif
}
