#include "FileSystem.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Define.h"

StaticJsonDocument<256> doc;

struct WiFiConfig {
  char AccsessPoint[40] = "Lights.Camera.Action";
  char Passcode[40] = "Passsword10";
  char Host[40] ="ESP32PLC-1234";
  char DHCP = 1;
  char IP[22] = "000.000.000.000";
  char DefultGateway[22] = "000.000.000.000";
  char SubMask[22] = "000.000.000.000";
};

struct MQTTConfig {
  char MQTTEnabble = 1;
  char MQTTIP[22] = "000.000.000.000";;
  char MQTTPassword[40] = "Stuff";
};

const char *filename = "/config.txt";  // <- SD library uses 8.3 filenames
WiFiConfig wfconfig;                         // <- global configuration object

void listFilesInDir(File dir, int numTabs);
void saveConfiguration();
void loadConfiguration();

void StartFileSystem(void){
    if(!SPIFFS.begin(true)){
        Serial.println("File Sytem Failed TO MOUNT");
    }
    else{
        File dir = SPIFFS.open("/");
        // List file at root
        listFilesInDir(dir, 1);

        SPIFFS.format();

        if(!SPIFFS.exists("/config.json")){
            //Config Doc dson't exist, wite one!
            Serial.println("No Config! Save One!");
            saveConfiguration();
            Serial.println("Load Config!");
            loadConfiguration();
        }
        else{
            loadConfiguration();
        }
    }
}

// Loads the configuration from a file
void loadConfiguration() {
  // Open file for reading
  File file = SPIFFS.open(filename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println(F("Failed to read file, using default configuration"));
  }
  // Copy values from the JsonDocument to the Config
    strlcpy(wfconfig.AccsessPoint,doc["AccsessPoint"],sizeof(wfconfig.AccsessPoint));  
    strlcpy(wfconfig.Passcode,doc["Passcode"],sizeof(wfconfig.Passcode));
    strlcpy(wfconfig.Host,doc["Host"],sizeof(wfconfig.Host));
    strlcpy(wfconfig.IP,doc["IP"],sizeof(wfconfig.IP));
    strlcpy(wfconfig.DefultGateway,doc["DefultGateway"],sizeof(wfconfig.DefultGateway));
    strlcpy(wfconfig.SubMask,doc["SubMask"],sizeof(wfconfig.SubMask));
  // Close the file (Curiously, File's destructor doesn't close the file)
  //SPIFFS.close();
  file.close();
}

// Saves the configuration to a file
void saveConfiguration() {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["AccsessPoint"] = wfconfig.AccsessPoint;
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

  // Close the file
  //SPIFFS.close();
  file.close();
}


void listFilesInDir(File dir, int numTabs) {
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
}