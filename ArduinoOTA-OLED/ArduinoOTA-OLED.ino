#include "Functions.h"
#include "Oled.h"

unsigned long lastMsg = 0;
unsigned long lastUpdate = 0;
int value = 0;
char PinLastVal = 0;



// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  GPIOInit();
  SystemStart();
  ClientIdCreation();
  DisplayInit();

//    // Force WPA secured WiFi for the software access point.
//    // Because OTA is remote code execution (RCE) by definition, the password
//    // should be kept secret. By default, WiFiSettings will become an insecure
//    // WiFi access point and happily tell anyone the password. The password
//    // will instead be provided on the Serial connection, which is a bit safer.
//    WiFiSettings.secure = false;
//
//    // Set callbacks to start OTA when the portal is active
//    WiFiSettings.onPortal = []() {
//        setup_ota();
//    };
//    WiFiSettings.onPortalWaitLoop = []() {
//        ArduinoOTA.handle();
//    };
//
//    // Use stored credentials to connect to your WiFi access point.
//    // If no credentials are stored or if the access point is out of reach,
//    // an access point will be started with a captive portal to configure WiFi.
//    WiFiSettings.connect();
//
//    Serial.print("Password: ");
//    Serial.println(WiFiSettings.password);
//    setup_ota();  // If you also want the OTA during regular execution
//    Serial.println(WiFi.localIP());
}

void loop() {
  WiFiConnectAnimation(100);
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
