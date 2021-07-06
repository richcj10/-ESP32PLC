#include "Functions.h"
#include "Oled.h"
#include "Sensors.h"
#include "MQTT.h"

unsigned long lastMsg = 0;
unsigned long lastRS485 = 0;
unsigned long lastUpdate = 0;
String clientId = "";
char PinLastVal = 0;

char WiFiConnected = 0;

void SystemStart(){
  Serial.begin(115200);
  Serial1.begin(115200,SERIAL_8N1,RXD2,TXD2);
  SPIFFS.begin(true);  // Will format on the first run after failing to mount
  Wire.begin(21, 22);   // sda= GPIO_21 /scl= GPIO_22 
}

void GPIOInit(){
  pinMode(2, OUTPUT);
  pinMode(26, INPUT);
  pinMode(DIR, OUTPUT);
  digitalWrite(DIR, HIGH);
}

void SyncLoop(){
  //ArduinoOTA.handle();
  unsigned long now = millis();
  if (now - lastUpdate > 700){
    lastUpdate = now;
    digitalWrite(LED,!digitalRead(LED));
    WiFiCheckRSSI();
    ReadDS18B20OneWire();
    readDeviceClimate();
    DisplayCenterChestTemp();
    char PinValue = digitalRead(26);
    if(PinValue != PinLastVal){
      if(PinValue == LOW){
        SendChestPower(1);
      }
      else{
        SendChestPower(0);
      }
      PinLastVal = PinValue;
    }
    if ((WiFi.status() != WL_CONNECTED)) {
      Serial.println("Rebooting - Lost WiFi");
      delay(1000);
      ESP.restart();
    }
  }
  if (now - lastMsg > 10000){
    lastMsg = now;
    SendDeviceEnviroment();
    SendChestFreezer();
    DisplayTHBar();
  }
  if (now - lastRS485 > 1000){
    lastRS485 = now;
    Serial1.print(0xAA,HEX);
    Serial1.print(0xFF,HEX);
  }
}

char LastWiFiSig = 0;

void WiFiCheckRSSI(){
  long Rssi = WiFi.RSSI()*-1;
  if(Rssi > HIGHRSSI){
    if(LastWiFiSig != 1){
      WiFiStreanthDisplay(1);
      LastWiFiSig = 1;
    }
  }
  else if((Rssi < HIGHRSSI) && (Rssi > LOWRSSI)){
    if(LastWiFiSig != 2){
      WiFiStreanthDisplay(2);
      LastWiFiSig = 2;
    }
  }
  else if(Rssi < LOWRSSI){
    if(LastWiFiSig != 3){
      WiFiStreanthDisplay(3);
      LastWiFiSig = 3;
    }
  }
}

void setup_ota() {
  ArduinoOTA.setHostname(GetClientId().c_str());
  ArduinoOTA.setPassword(WiFiSettings.password.c_str());
  ArduinoOTA.begin();
}

void ConnectUpdate(){
  digitalWrite(LED,!digitalRead(LED));
  WiFiConnectAnimation();
}

void WiFiFaiure(){
  WiFiAP(1);
  WiFiConnected = 0;
}

void WiFiStart(void){
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(clientId.c_str());
  //WiFiSettings.hostname(clientId);
  WiFiSettings.secure = false;
  WiFiSettings.onWaitLoop = []() {ConnectUpdate(); return 100; };
  //WiFiSettings.onSuccess  = []() {  };
  WiFiSettings.onFailure  = []() { WiFiFaiure(); };
  WiFiSettings.onPortal = []() {
    setup_ota();
  };
  WiFiSettings.onPortalWaitLoop = []() {
    ArduinoOTA.handle();
  };
  
  WiFiSettings.connect();
  
  Serial.print("Password: ");
  Serial.println(WiFiSettings.password);
  WiFiStreanthDisplay(3);
  WiFiConnected = 1;
  setup_ota();  // If you also want the OTA during regular execution
  Serial.println(WiFi.localIP());
}

char GetWiFiStatus(){
  return(WiFiConnected);
}

String GetClientId(){
  return(clientId);
}

void ClientIdCreation(void){
  byte mac[6];
  WiFi.macAddress(mac);
  clientId = "ESPPLC-";
  clientId = clientId + String(mac[1]) + String(mac[0]);
  
  Serial.print("ClientID = ");
  Serial.println(clientId);
}
