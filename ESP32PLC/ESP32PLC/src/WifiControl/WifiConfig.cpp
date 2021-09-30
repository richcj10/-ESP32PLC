#include <Arduino.h>
#include <WiFi.h>
#include "WifiControl/WifiConfig.h"
#include "FileSystem/FSInterface.h"
#include "Define.h"
#include "Functions.h"

char SetupWiFi(void){
    if(setupMode() == 1){
        Serial.printf("WiFi STA mode!\n");
        WiFi.disconnect();
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(GetHostName().c_str());
        delay(250);
        WiFi.mode(WIFI_AP);
        delay(250);
        Serial.println(GetSSID().c_str());
        WiFi.begin(GetSSID().c_str(), GetSSIDPassword().c_str());
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print('.');
            delay(1000);
        }

        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return 1;
    }
    else if(setupMode() == 2){
        Serial.printf("WiFi AP mode!\n");
        WiFi.disconnect();
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(GetHostName().c_str());
        delay(250);
        WiFi.mode(WIFI_AP);
        delay(250);
        WiFi.softAP(GetClientId().c_str());
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP()); 
        return 1;       
    }
    else{
        Serial.println("WiFi OFF");
        return 1;
    }
    return 2;
}

char setupMode(void){
    return 1;
}