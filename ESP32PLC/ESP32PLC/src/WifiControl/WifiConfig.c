#include "WifiConfig.h"
#include "debug.h"
#include <Arduino.h>

char setupMode();

char SetupWiFi(void){
    if(setupMode() == 1){
        //WiFi.mode(WIFI_STA);
        //WiFi.begin(ssid, password);
        //if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            LOG("WiFi Failed!\n");
            //return 0;
        //}

        //Serial.print("IP Address: ");
        //Serial.println(WiFi.localIP());
        //return 1;
    }
    else if(setupMode() == 2){
        //WiFi.mode(WIFI_AP);
        //WiFi.begin(ssid, password);
        //if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            //Serial.printf("WiFi Failed!\n");
        //    return 0;
        //}

        //Serial.print("IP Address: ");
        //Serial.println(WiFi.localIP()); 
        //return 1;       
    }
    else{
        //Serial.println("WiFi OFF");
        return 1;
    }
}

char setupMode(){
    return 2;
}