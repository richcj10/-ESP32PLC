#include "MQTT.h"
#include "Functions.h"
#include "Oled.h"
#include "Sensors.h"
#include <PubSubClient.h>

#define MQTTid "123321"
#define MQTTip "192.168.5.4"
#define MQTTport 1883
#define MQTTuser ""
#define MQTTpsw ""
#define MQTTpubQos 1
#define MQTTsubQos 1

#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];
char MQTTActive = 0;
char temp[50]; 

WiFiClient wclient;
PubSubClient client(wclient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void MqttLoop(void){
  if(client.connected()){
    client.loop();
  }
  else{
    MQTTreconnect();
  }
}

void MQTTStart(){
  client.setServer(MQTTip, 1883);
  client.setCallback(callback);
}

void MQTTreconnect(void) {
  // Loop until we're reconnected
    MQTTIconSet(0);
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if(client.connect(GetClientId().c_str())) {
        MQTTActive = 1;
        MQTTIconSet(1);
        Serial.println("MQTT connected!");
    }
  }
}

void SendDeviceEnviroment(){
  readDeviceClimate();
  printInfo();
  String TempMesure = String(getDeviceClimateTemprature());
  TempMesure.toCharArray(temp, TempMesure.length() + 1);
  if(client.publish("home/garage/cf/device/temp", temp)){
    Serial.println("Sent Device Temp");
  }
  TempMesure = String(getDeviceClimateHumidity());
  TempMesure.toCharArray(temp, TempMesure.length() + 1);
  if(client.publish("home/garage/cf/device/humid", temp)){
    Serial.println("Sent Device Humidity");
   }
}

void SendChestFreezer(){
  readDeviceClimate();
  printInfo();
  String TempMesure = String(getDeviceClimateTemprature());
  TempMesure.toCharArray(temp, TempMesure.length() + 1);
  if(client.publish("home/garage/cf/temp", temp)){
    Serial.println("Sent CF Temp");
  }
  if(!digitalRead(MP1INPUT)){
    String temp = "ON";
  }
  else{
    String temp = "OFF";
  }
  if(client.publish("home/garage/cf/power", temp)){
    Serial.println("Sent Device Power");
   }
}
