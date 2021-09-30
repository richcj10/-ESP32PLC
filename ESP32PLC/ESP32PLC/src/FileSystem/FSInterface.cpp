#include "FSInterface.h"
#include "FileSystem.h"
#include "Arduino.h"

WiFiConfig wfconfig;                         // <- global configuration object
MQTTConfig mqconfig;                         // <- global configuration object

char FileStstemStart(){
    char result = FileSystemInit(&wfconfig,&mqconfig);
    if(result == 1){
        return 1;
    }
    return 0;
}

String GetSSID(){
    char ReturnArray[wfconfig.SSIDLN];
    for(unsigned char i = 0;i<=wfconfig.SSIDLN;i++){
        ReturnArray[i] = wfconfig.SSID[i];
    }
    String ReturnString = String(ReturnArray);
    return ReturnString;
}

String GetSSIDPassword(){
    char ReturnArray[wfconfig.PswdLN];
    for(unsigned char i = 0;i<=wfconfig.PswdLN;i++){
        ReturnArray[i] = wfconfig.Passcode[i];
    }
    String ReturnString = String(ReturnArray);
    return ReturnString;
}

String GetHostName(){
    char ReturnArray[wfconfig.HoastLN];
    for(unsigned char i = 0;i<=wfconfig.HoastLN;i++){
        ReturnArray[i] = wfconfig.Host[i];
    }
    String ReturnString = String(ReturnArray);
    return ReturnString;
}