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
char MQTTLockout = 0;
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
  if(MQTTLockout == 0){
    if(client.connected()){
      client.loop();
    }
    else{
      MQTTreconnect();
    }
  }
}

void MQTTStart(){
  client.setServer(MQTTip, 1883);
  client.setCallback(callback);
}

void MQTTreconnect(void) {
  // Loop until we're reconnected
  char counter = 0;
    if(GetWiFiStatus() == 1){
      MQTTIconSet(0);
      while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if(client.connect(GetClientId().c_str())) {
          MQTTActive = 1;
          MQTTIconSet(1);
          Serial.println("MQTT connected!");
          MQTTMessageInit();
       }
       counter++;
       if(counter > 2){
         Serial.println("MQTT ISSUE");
         MQTTIconSet(0);
         MQTTLockout = 1;
         break;
       }
     }
   }
}

void MQTTMessageInit(){
  SendChestFreezer();
  SendChestPower(!digitalRead(26));
}

void SendDeviceEnviroment(){
  if(MQTTActive == 1){
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
}

void SendChestFreezer(){
  if(MQTTActive == 1){
    ReadDS18B20OneWire();
    String TempMesure = String(getOneWireTemprature());
    TempMesure.toCharArray(temp, TempMesure.length() + 1);
    if(client.publish("home/garage/cf/temp", temp)){
      Serial.println("Sent CF Temp");
    }
    else{
      Serial.println("fail CF Temp");
    }
  }
}

void SendChestPower(char Mode){
  if(MQTTActive == 1){
    if(Mode == 1){
      if(client.publish("home/garage/cf/power", "ON")){
        Serial.println("Sent CF Power - ON");
      }
    }
    if(Mode == 0){
      if(client.publish("home/garage/cf/power", "OFF")){
        Serial.println("Sent CF Power - OFF");
      }
    }
  }
}


char GetMQTTStatus(void){
  return MQTTActive;
}
