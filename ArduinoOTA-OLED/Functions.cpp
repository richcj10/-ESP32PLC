#include "Functions.h"
#include "Oled.h"
#include "Sensors.h"
#include "MQTT.h"

unsigned long lastMsg = 0;
unsigned long lastUpdate = 0;
String clientId = "";

void SystemStart(){
  Serial.begin(115200);
  SPIFFS.begin(true);  // Will format on the first run after failing to mount
  Wire.begin(21, 22);   // sda= GPIO_21 /scl= GPIO_22 
}

void GPIOInit(){
  pinMode(LED, OUTPUT);
  pinMode(MP1INPUT, INPUT);
}

void SyncLoop(){
  unsigned long now = millis();
  if (now - lastUpdate > 500){
    digitalWrite(LED,!digitalRead(LED));
    ReadDS18B20OneWire();
  }
  if (now - lastMsg > 1000){
    SendDeviceEnviroment();
    SendChestFreezer();
  }
}

void setup_ota() {
    ArduinoOTA.setHostname(clientId.c_str());
    ArduinoOTA.setPassword(WiFiSettings.password.c_str());
    ArduinoOTA.begin();
}

void ConnectUpdate(){
  digitalWrite(LED,!digitalRead(LED));
  WiFiConnectAnimation();
}

void WiFiStart(void){
  WiFiSettings.secure = false;
  WiFiSettings.onWaitLoop = []() {ConnectUpdate(); return 100; };
  //WiFiSettings.onSuccess  = []() { WiFiStreanth(3); };
  //WiFiSettings.onFailure  = []() { WiFiStreanth(0); };
  WiFiSettings.onPortal = []() {
    setup_ota();
  };
  WiFiSettings.onPortalWaitLoop = []() {
    ArduinoOTA.handle();
  };
  
  WiFiSettings.connect();
  
  Serial.print("Password: ");
  Serial.println(WiFiSettings.password);
  setup_ota();  // If you also want the OTA during regular execution
  Serial.println(WiFi.localIP());
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
