#include "Functions.h"
#include "Oled.h"
#include "MQTT.h"
#include "Sensors.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  GPIOInit();
  SystemStart();
  InitSensors();
  ClientIdCreation();
  DisplayInit();
  WiFiStart();
  MQTTStart();
  Serial.println("Setup Done!");
}

void loop() {
  //WiFiConnectAnimation(100);
  MqttLoop();
  SyncLoop();
  //Serial.println("Running");
//  ArduinoOTA.handle();  // If you also want the OTA during regular execution
//  MqttLoop();
//  unsigned long now = millis();
//  if (now - lastUpdate > 500) {
//    lastUpdate = now;
//    SetDisplayInit();
//    getWeather();
//    digitalWrite(LED, !digitalRead(LED));
//    char PinValue = digitalRead(26);
//    if(PinValue != PinLastVal){
//      if(PinValue == LOW){
//        if(client.publish("home/garage/cf/power", "ON")){
//          Serial.println("Sent Chest AC -ON");
//        }
//      }
//      else{
//        if(client.publish("home/garage/cf/power", "OFF")){
//          Serial.println("Sent Chest AC -OFF");
//        }
//      }
//      PinLastVal = PinValue;
//    }   
//  }
//  if (now - lastMsg > 10000) {
//    lastMsg = now;
//    String TempMesure = String(tempf);
//    char temp[50]; 
//    TempMesure.toCharArray(temp, TempMesure.length() + 1);
//    if(client.publish("home/garage/cf/temp", temp)){
//      Serial.println("Sent Chest Temp");
//     }
//   }
}
