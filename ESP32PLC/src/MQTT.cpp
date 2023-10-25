#include "MQTT.h"
#include "Functions.h"
#include "Display/Oled.h"
#include "Sensors.h"
#include <PubSubClient.h>
#include "Remote/MasterController.h"

#define MQTTid "123321"
#define MQTTip "192.168.10.101"
#define MQTTport 1883
#define MQTTuser "ESPPLC"
#define MQTTpsw "ESPPLCpass"
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
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  if (String(topic) == "ESPPLC/Heater") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(8, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(8, LOW);
    }
  }
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
      while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if(client.connect(GetClientId().c_str(),MQTTuser,MQTTpsw)) {
          MQTTActive = 1;
          Serial.println("MQTT connected!");
          client.subscribe("ESPPLC/Heater");
          //MQTTMessageInit();
       }
       counter++;
       if(counter > 2){
         Serial.println("MQTT ISSUE");
         MQTTLockout = 1;
         break;
       }
     }
   }
}

/* void MQTTMessageInit(){
  SendChestFreezer();
  SendChestPower(!digitalRead(26));
} */

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

void SendOutsideEnvoroment(){
  if(MQTTActive == 1){
    String report;
    report = String(GetRemoteDataFromQue(OUTSIDE_TEMP_POS,1));
    report.toCharArray(temp, report.length() + 1);
    if(client.publish("Outside/Weather/temp", temp) == 0){
      Serial.println("MQTT Send Fail");
    }
    report = String(GetRemoteDataFromQue(OUTSIDE_HUMID_POS,1));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/Weather/humid", temp);
    report = String(GetRemoteDataFromQue(MC_POS,1));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/Weather/mH", temp);
    report = String(GetRemoteDataFromQue(WIND_SPEED_POS,1));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/Weather/WindSpeed", temp);
    report = String(GetRemoteDataFromQue(WIND_DIR_POS,0));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/Weather/windDir", temp);
/*     report = String(GetRemoteDataFromQue(OUTSIDE_TEMP_POS,1));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/Weather/Rain", temp); */
  }
}

void SendRemoteRTD(){
  if(MQTTActive == 1){
    String report;
    report = String(GetRemoteDataFromQue(REMOTE_TEMP_RTD_1,1));
    report.toCharArray(temp, report.length() + 1);
    if(client.publish("Outside/UnderRV/RTD1", temp) == 0){
      Serial.println("MQTT Send Fail");
    }
    report = String(GetRemoteDataFromQue(REMOTE_TEMP_RTD_2,1));
    report.toCharArray(temp, report.length() + 1);
    if(client.publish("Outside/UnderRV/RTD2", temp) == 0){
      Serial.println("MQTT Send Fail");
    }
    report = String(GetRemoteDataFromQue(REMOTE_TEMP_RTD_3,1));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/UnderRV/RTD3", temp);
  }
}

void SendRemoteCurrentSense(){
  if(MQTTActive == 1){
    String report;
    report = String(GetRemoteDataFromQue(REMOTE_CS_A,1));
    report.toCharArray(temp, report.length() + 1);
    if(client.publish("Outside/Current/A", temp) == 0){
      Serial.println("MQTT Send Fail");
    }
    report = String(GetRemoteDataFromQue(REMOTE_CS_B,1));
    report.toCharArray(temp, report.length() + 1);
    if(client.publish("Outside/Current/B", temp) == 0){
      Serial.println("MQTT Send Fail");
    }
    report = String(GetRemoteDataFromQue(REMOTE_CS_C,1));
    report.toCharArray(temp, report.length() + 1);
    client.publish("Outside/Current/C", temp);
  }
}

char GetMQTTStatus(void){
  return MQTTActive;
}
