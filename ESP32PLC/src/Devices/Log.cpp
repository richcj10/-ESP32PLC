#include "Log.h"
#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>
#include "Webportal.h"

bool ConfigArray[5] = {1,1,1,1,0};
bool WiFiLog = 0;

void LogSetup(char DebugLevel, bool WebPage){
    switch (DebugLevel){
        case ERROR:
            ConfigArray[0] = 1;
            ConfigArray[1] = 0;
            ConfigArray[2] = 0;
            ConfigArray[3] = 0;
            break;
        case LOG:
            ConfigArray[0] = 1;
            ConfigArray[1] = 1;
            ConfigArray[2] = 0;
            ConfigArray[3] = 0;
            break;
        case NOTIFY:
            ConfigArray[0] = 1;
            ConfigArray[1] = 1;
            ConfigArray[2] = 1;
            ConfigArray[3] = 0;
            break;
        case DEBUG:
            ConfigArray[0] = 1;
            ConfigArray[1] = 1;
            ConfigArray[2] = 1;
            ConfigArray[3] = 1;
            break;
    }
    if(WebPage){
        WiFiLog = 1;
    }
}

char Log(char level, const char* format, ...){
    switch (level){
        case ERROR:
            if(ConfigArray[0] == 1){
                ets_printf("%s", "ERR>");
            }
            else{
                return 0; 
            }
            break;
        case LOG:
            if(ConfigArray[1] == 1){
                ets_printf("%s", "LOG>");
            }
            else{
                return 0; 
            }
            break;
        case NOTIFY:
            if(ConfigArray[2] == 1){
                ets_printf("%s", "NOTIFY>");
            }
            else{
                return 0; 
            }
            break;
        case DEBUG:
            if(ConfigArray[3] == 1){
                ets_printf("%s", "DEBUG>");
            }
            else{
                return 0; 
            }
            break;
        case NOTIFY_FORCE:
            ets_printf("%s", "NOTIFY_F>");
            break;
        default:
            ets_printf("%s", "ALT>");
            break;
    }
    static char loc_buf[64];
    char * temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = (char*)malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            //return 0;
        }
    }
    
    vsnprintf(temp, len+1, format, arg);

    ets_printf("%s", temp);
    if(WiFiLog){
        String s = temp;
        WebLogSend(s);
    }
    va_end(arg);
    if(len >= sizeof(loc_buf)){
        free(temp);
    }
    return 0;
}