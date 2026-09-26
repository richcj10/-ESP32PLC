#pragma once

// Start the boot-button watcher (background task, first 10 s after boot).
// Hold USER_SW 5 s = network reset, 15 s = full factory reset; both restart
// into an open AP. Call once, after StatusLEDStart() and Serial/Log setup.
void BootResetStart();
