// The only file that includes the generated fw_version.h (scripts/version.py).
// A new commit, or the "-dirty" flag appearing, changes that header — keeping it
// here means only this tiny file recompiles, not the large Webportal.cpp.
#include "FwVersion.h"

#if __has_include("fw_version.h")
#include "fw_version.h"
#endif
#ifndef FW_VERSION
#define FW_VERSION  "dev"
#endif
#ifndef FW_SHA
#define FW_SHA      "0000000"
#endif
#ifndef WEB_VERSION
#define WEB_VERSION "0000000"
#endif

const char* GetFwVersion()  { return FW_VERSION; }
const char* GetFwSha()      { return FW_SHA; }
const char* GetWebVersion() { return WEB_VERSION; }
