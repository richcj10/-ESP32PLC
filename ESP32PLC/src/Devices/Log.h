#ifndef LOG_H
#define  LOG_H

#define ERROR 1
#define LOG 2
#define NOTIFY 3
#define DEBUG 4
#define NOTIFY_FORCE 5 //Used to push to connsol, regardless of debug level 

void LogSetup(char DebugLevel, bool WebPage);
char Log(char level,const char* format, ...);

#endif 