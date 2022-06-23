#include <Arduino.h>
#include <WiFi.h>
#include "WifiControl/WifiConfig.h"
#include "FileSystem/FSInterface.h"
#include "Define.h"
#include "Functions.h"
#include "Devices/StatusLED.h"
#include "Display/Display.h"

String IpAddress2String(const IPAddress& ipAddress);

const char* ssid     = "Lights.Camera.Action";
const char* password = "RR58fa!8";

char SetupWiFi(void){
    if(setupMode() == 1){
        Serial.printf("WiFi STA mode!\n");
        //WiFi.disconnect();
        //WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        //WiFi.setHostname(GetHostName().c_str());
        Serial.println(GetSSID().c_str());
        SetLEDStatus(WIFI_CONNECTING,250);
        //WiFi.begin(GetSSID().c_str(), GetSSIDPassword().c_str());
        WiFi.begin(ssid, password);
        int counter = 0;
        while (WiFi.status() != WL_CONNECTED) {
            LEDUpdate();
            DisplayWiFiConnect();
            counter++;
            delay(50);
            if(counter > 140){
                ESP.restart();
            }
        }

        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        GetIPStr();
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

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

char setupMode(void){
    return 1;
}

String GetIPStr(){
    return IpAddress2String(WiFi.localIP());
}

String GetRSSIStr(){
    return String(WiFi.RSSI());
}

String GetMACStr(){
    byte mac[6];
    WiFi.macAddress(mac);
    return String(mac[5]) +":"+ String(mac[4]) +":"+ String(mac[3]) +":"+ String(mac[2]) +":"+ String(mac[1]) +":"+ String(mac[0]);
}