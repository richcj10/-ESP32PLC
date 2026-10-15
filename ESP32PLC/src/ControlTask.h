#pragma once
#include <stdint.h>

// High-priority task for inputs, Modbus and realtime modules — see ControlTask.cpp.
// Start once, after RemoteStart(), IOStart() and modules.begin().
void StartControlTask();

// Control-task timing since the last call (resets the window), for /api/mem.
void ControlStatsRead(uint32_t* maxUs, uint32_t* peakUs, uint32_t* avgUs, uint32_t* perSec);
