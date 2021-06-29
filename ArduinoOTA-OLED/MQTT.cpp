#include "MQTT.h"
#include "Functions.h"
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
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if(client.connect(GetClientId().c_str())) {
        MQTTActive = 1;
    }
  }
}

void SendDeviceEnviroment(){
//  TempMesure = String(tempf);
//  TempMesure.toCharArray(temp, TempMesure.length() + 1);
//  if(client.publish("home/garage/cf/device/temp", temp)){
//    Serial.println("Sent Device Temp");
//  }
//  TempMesure = String(humidity);
//  TempMesure.toCharArray(temp, TempMesure.length() + 1);
//  if(client.publish("home/garage/cf/device/humid", temp)){
    Serial.println("Sent Device Humidity");
//   }
}
