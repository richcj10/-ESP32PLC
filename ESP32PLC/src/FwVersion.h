#pragma once

// Git-derived versions, generated at build time by scripts/version.py.
const char* GetFwVersion();    // e.g. "0279052-dirty"
const char* GetFwSha();        // short commit hash
const char* GetWebVersion();   // last commit touching data/
