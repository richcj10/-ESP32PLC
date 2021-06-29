/*
    WifiSettings Arduino OTA example

    Demonstrates how to run Arduino OTA in tandem with WifiSettings,
    using the WifiSettings credentials.

    Source and further documentation available at
    https://github.com/Juerd/ESP-WiFiSettings

    Note: this example is written for ESP32.
    For ESP8266, use LittleFS.begin() instead of SPIFFS.begin(true).
*/

#include <SPIFFS.h>
#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>
#include "SparkFun_Si7021_Breakout_Library.h"
#include <DS18B20.h>
DS18B20 ds(5);
uint8_t address[] = {40, 250, 31, 218, 4, 0, 0, 52};
uint8_t selected;

#define MQTTid "123321"
#define MQTTip "192.168.5.4"
#define MQTTport 1883
#define MQTTuser ""
#define MQTTpsw ""
#define MQTTpubQos 1
#define MQTTsubQos 1

#define LED 2

byte mac[6];
unsigned long lastMsg = 0;
unsigned long lastUpdate = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
String clientId = "";
float humidity = 0;
float tempf = 0;
char PinLastVal = 0;

Weather sensor;
WiFiClient wclient;
PubSubClient client(wclient);

void SetDisplayInit(void);
void getWeather();

// Start ArduinoOTA via WiFiSettings with the same hostname and password
void setup_ota() {
    ArduinoOTA.setHostname(WiFiSettings.hostname.c_str());
    ArduinoOTA.setPassword(WiFiSettings.password.c_str());
    ArduinoOTA.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void ClientIdCreation(){
  WiFi.macAddress(mac);
  clientId = "ESPPLC-";
  clientId = clientId + String(mac[1]) + String(mac[0]);
  
  Serial.print("ClientID = ");
  Serial.println(clientId);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      String TempMesure = String(tempf);
      char temp[50];
      TempMesure.toCharArray(temp, TempMesure.length() + 1);
      Serial.print("Publish message: ");
      Serial.println(temp);
      client.publish("home/garage/cf/temp", temp);
      // ... and resubscribe
      if(client.subscribe("home/garage/cf/light")){
        Serial.println("Sent");
      }
      char PinValue = digitalRead(26);
      if(PinValue == LOW){
        if(client.publish("home/garage/cf/power", "ON")){
          Serial.println("Sent Chest AC -ON");
        }
      }
      if(PinValue == HIGH){
        if(client.publish("home/garage/cf/power", "OFF")){
          Serial.println("Sent Chest AC -OFF");
        }
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    pinMode(26, INPUT);
    digitalWrite(LED, HIGH);
    SPIFFS.begin(true);  // Will format on the first run after failing to mount

    // Force WPA secured WiFi for the software access point.
    // Because OTA is remote code execution (RCE) by definition, the password
    // should be kept secret. By default, WiFiSettings will become an insecure
    // WiFi access point and happily tell anyone the password. The password
    // will instead be provided on the Serial connection, which is a bit safer.
    WiFiSettings.secure = false;

    // Set callbacks to start OTA when the portal is active
    WiFiSettings.onPortal = []() {
        setup_ota();
    };
    WiFiSettings.onPortalWaitLoop = []() {
        ArduinoOTA.handle();
    };

    // Use stored credentials to connect to your WiFi access point.
    // If no credentials are stored or if the access point is out of reach,
    // an access point will be started with a captive portal to configure WiFi.
    WiFiSettings.connect();

    Serial.print("Password: ");
    Serial.println(WiFiSettings.password);
    setup_ota();  // If you also want the OTA during regular execution
    Serial.println(WiFi.localIP());
    ClientIdCreation();
    selected = ds.select(address);

    if (selected) {
      //ds.setAlarms(LOW_ALARM, HIGH_ALARM);
      Serial.println("Device Found!");
    } else {
      Serial.println("Device not found!");
    }
    Wire.begin(21, 22);   // sda= GPIO_21 /scl= GPIO_22 
    oled.init();                      // Initialze SSD1306 OLED display
    oled.clearDisplay();              // Clear screen
    client.setServer(MQTTip, 1883);
    client.setCallback(callback);
    sensor.begin();
    SetDisplayInit();
}

void loop() {
    ArduinoOTA.handle();  // If you also want the OTA during regular execution
    if(client.connected()){
      client.loop();
    }
    else{
      reconnect();
    }
  unsigned long now = millis();
  if (now - lastUpdate > 500) {
    lastUpdate = now;
    SetDisplayInit();
    getWeather();
    Serial.print("Ds Temp:");
    Serial.println(ds.getTempC());
    digitalWrite(LED, !digitalRead(LED));
    char PinValue = digitalRead(26);
    if(PinValue != PinLastVal){
      if(PinValue == LOW){
        if(client.publish("home/garage/cf/power", "ON")){
          Serial.println("Sent Chest AC -ON");
        }
      }
      else{
        if(client.publish("home/garage/cf/power", "OFF")){
          Serial.println("Sent Chest AC -OFF");
        }
      }
      PinLastVal = PinValue;
    }   
  }
  if (now - lastMsg > 10000) {
    lastMsg = now;
    String TempMesure = String(tempf);
    char temp[50]; 
    TempMesure.toCharArray(temp, TempMesure.length() + 1);
    if(client.publish("home/garage/cf/temp", temp)){
      Serial.println("Sent Chest Temp");
     }
    TempMesure = String(tempf);
    TempMesure.toCharArray(temp, TempMesure.length() + 1);
    if(client.publish("home/garage/cf/device/temp", temp)){
      Serial.println("Sent Device Temp");
     }
    TempMesure = String(humidity);
    TempMesure.toCharArray(temp, TempMesure.length() + 1);
    if(client.publish("home/garage/cf/device/humid", temp)){
      Serial.println("Sent Device Humidity");
     }
   }
}

void getWeather()
{
  // Measure Relative Humidity from the HTU21D or Si7021
  humidity = sensor.getRH();

  // Measure Temperature from the HTU21D or Si7021
  tempf = sensor.getTempF();
  // Temperature is measured every time RH is requested.
  // It is faster, therefore, to read it from previous RH
  // measurement with getTemp() instead with readTemp()
}
//---------------------------------------------------------------
void printInfo()
{
//This function prints the weather data out to the default Serial Port

  Serial.print("Temp:");
  Serial.print(tempf);
  Serial.print("F, ");

  Serial.print("Humidity:");
  Serial.print(humidity);
  Serial.println("%");
}

void SetDisplayInit(void) {
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString(WiFi.localIP().toString().c_str());
  oled.setTextXY(1,12);              // Set cursor position, start of line 0
  oled.putString(String(WiFi.RSSI()));
  oled.setTextXY(1,2);              // Set cursor position, start of line 0
  oled.putString("T:");
  oled.setTextXY(1,5);
  oled.putString(String(tempf));
  oled.setTextXY(2,2);
  oled.putString("H:");
  oled.setTextXY(2,5);
  oled.putString(String(humidity));
}
