#include "FileSystem.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Define.h"

struct WiFiConfig {
  char SSID[40] = "L.C.A";
  char Passcode[40] = "Hi";
  char Host[40] ="ESP32PLC-1234";
  char DHCP = 1;
  char IP[22] = "192.168.5.10";
  char DefultGateway[22] = "000.000.000.000";
  char SubMask[22] = "000.000.000.000";
};

struct MQTTConfig {
  char MQTTEnabble = 1;
  char MQTTIP[22] = "000.000.000.000";;
  char MQTTPassword[40] = "Stuff";
};

const char *WiFifilename = "/WiFiconfig.json";  // <- SD library uses 8.3 filenames
const char *MQTTfilename = "/MQTTconfig.json";  // <- SD library uses 8.3 filenames
const char *MQTTTopicsfilename = "/MQTTcTopics.json";  // <- SD library uses 8.3 filenames

WiFiConfig wfconfig;                         // <- global configuration object
MQTTConfig mqconfig;                         // <- global configuration object

void listFilesInDir(File dir, int numTabs);
void WifiComfig();
void MqttComfig();
void PrintMqttConfigStruct();

void StartFileSystem(void){
  if(SPIFFS.begin(true)){
    File dir = SPIFFS.open("/");
    listFilesInDir(dir, 1);

    //SPIFFS.format();
    LOG(" Config Check \r"); 
    // Serial.println(F(" Config Check   ")); 
    WifiComfig();
    MqttComfig();
  }
  else{
    LOG("File Sytem Failed to mount!!!!!! \r");
    SPIFFS.format();
    LOG("File Sytem Failed to mount ----Reboot \r");
    ESP.restart();
  }
}

void WifiComfig(){
  if(!SPIFFS.exists(WiFifilename)){
    //Config Doc dson't exist, wite one!
    LOG("No Wifi Config! Save One!....");
    WifisaveConfiguration();
    LOG("Now Load Config! \r");            
    WifiloadConfiguration();
  }
  else{
    LOG("Found File.....Load Config!\r");
    WifiloadConfiguration();
    delay(100);
    PrintWiFiConfigStruct();
    delay(100);
  }
}

void MqttComfig(){
  if(!SPIFFS.exists(MQTTfilename)){
    //Config Doc dson't exist, wite one!
    LOG("No MQTT Config! Save One!....");
    MqttsaveConfiguration();
    LOG("Now Load Config! \r");            
    MqttloadConfiguration();
  }
  else{
    LOG("Found File.....Load Config!\r");
    MqttloadConfiguration();
    delay(100);
    PrintMqttConfigStruct();
    delay(100);
  }
}

// Loads the configuration from a file
void WifiloadConfiguration() {
  // Open file for reading
  File file = SPIFFS.open(WiFifilename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println(F("Failed to read file, using default configuration"));
  }
  // Copy values from the JsonDocument to the Config
  strlcpy(wfconfig.SSID,doc["SSID"],sizeof(wfconfig.SSID));
  strlcpy(wfconfig.Passcode,doc["Passcode"],sizeof(wfconfig.Passcode));
  strlcpy(wfconfig.Host,doc["Host"],sizeof(wfconfig.Host));
  strlcpy(wfconfig.IP,doc["IP"],sizeof(wfconfig.IP));
  strlcpy(wfconfig.DefultGateway,doc["DefultGateway"],sizeof(wfconfig.DefultGateway));
  strlcpy(wfconfig.SubMask,doc["SubMask"],sizeof(wfconfig.SubMask));
  // Close the file (Curiously, File's destructor doesn't close the file)
  //SPIFFS.close();
  file.close();
}

void MqttloadConfiguration() {
  // Open file for reading
  File file = SPIFFS.open(MQTTfilename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println(F("Failed to read file, using default configuration"));
  }
  // Copy values from the JsonDocument to the Config
  ///strlcpy(mqconfig.MQTTEnabble,doc["MQTTEnabble"],sizeof(wfconfig.AccsessPoint));  
  strlcpy(mqconfig.MQTTIP,doc["MQTTIP"],sizeof(mqconfig.MQTTIP));
  strlcpy(mqconfig.MQTTPassword,doc["MQTTPassword"],sizeof(mqconfig.MQTTPassword));
  // Close the file (Curiously, File's destructor doesn't close the file)
  //SPIFFS.close();
  file.close();
}

// Saves the configuration to a file
void WifisaveConfiguration() {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(WiFifilename);

  // Open file for writing
  File file = SPIFFS.open(WiFifilename, "w");
  if (file) {
    LOG("Opened File! \r");
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<256> doc;

    // Set the values in the document
    doc["SSID"] = wfconfig.SSID;
    doc["Passcode"] = wfconfig.Passcode;
    doc["Host"] = wfconfig.Host;
    doc["IP"] = wfconfig.IP;
    doc["DefultGateway"] = wfconfig.DefultGateway;
    doc["SubMask"] = wfconfig.SubMask;
    // Serialize JSON to file
    serializeJsonPretty(doc, Serial);
    if (serializeJson(doc, file) == 0) {
      Serial.println(F("Failed to write to file"));
    }
    file.close();
  }
  else{
    LOG("File not able to be opened :(");
  }
}

void MqttsaveConfiguration() {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(MQTTfilename);

  // Open file for writing
  File file = SPIFFS.open(MQTTfilename, "w");
  if (file) {
    LOG("Opened MQTT File! \r");
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<256> doc;

    // Set the values in the document
    doc["MQTTIP"] = mqconfig.MQTTIP;
    doc["MQTTPassword"] = mqconfig.MQTTPassword;
    // Serialize JSON to file
    serializeJsonPretty(doc, Serial);
    if (serializeJson(doc, file) == 0) {
      Serial.println(F("Failed to write MQTT file"));
    }
    file.close();
  }
  else{
    LOG("MQTT File not able to be opened :(");
  }
}

void PrintWiFiConfigStruct(){
  Serial.print("AccsessPoint: ");
  volatile unsigned char i = 0;
  for(i = 0; i < 30; i++){
    Serial.print(wfconfig.SSID[i]);
  }
  Serial.println();
  Serial.print("Passcode: ");
  for(i = 0; i < sizeof(wfconfig.Passcode); i++){
    Serial.print(wfconfig.Passcode[i]);
  }
  Serial.println();
  Serial.print("Host: ");
  for(i = 0; i < 40; i++){
    Serial.print(wfconfig.Host[i]);
  }
  Serial.println();
  Serial.print("DHCP : "); Serial.println(wfconfig.DHCP);
  Serial.print("IP: ");
  for(i = 0; i < 22; i++){
    Serial.print(wfconfig.IP[i]);
  }
  Serial.println();
  Serial.print("DefultGateway: ");
  for(i = 0; i < 22; i++){
    Serial.print(wfconfig.DefultGateway[i]);
  }
  Serial.println();
  Serial.print("SubMask: ");
  for(i = 0; i < 22; i++){
    Serial.print(wfconfig.SubMask[i]);
  }
  Serial.println();
  Serial.println();
}

void PrintMqttConfigStruct(){
  Serial.print("Mqtt IP: ");
  volatile unsigned char i = 0;
  for(i = 0; i < 30; i++){
    Serial.print(mqconfig.MQTTIP[i]);
  }
  Serial.println();
  Serial.print(" MQtt Passcode: ");
  for(i = 0; i < sizeof(mqconfig.MQTTPassword); i++){
    Serial.print(mqconfig.MQTTPassword[i]);
  }
  Serial.println();
  Serial.println();
}

void listFilesInDir(File dir, int numTabs) {
  Serial.println("\r Files: \r");
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files in the folder
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFilesInDir(entry, numTabs + 1);
    } else {
      // display size for file, nothing for directory
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
  Serial.println("\r");
}