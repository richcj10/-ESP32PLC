#include "Functions.h"
#include "Display/Oled.h"
#include "MQTT.h"
#include "Sensors.h"
#include "Webportal.h"
#include "Display/Display.h"
#include "HAL/DeviceConfig.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "HAL/Digital/Digital.h"







// Start ArduinoOTA via WiFiSettings with the same hostname and password

void setup() {
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  SystemStart();
  ClientIdCreation();
  SaveResetReason();
  FileStstemStart();
  QueryLocalDevice();
  DisplaySetup(TFT);
  //GPIOStart();
  setupMode();
  //SetupWiFi();
  //InitSensors();
  //WiFiStart();
  //MQTTStart();
  //WebStart();
  Serial.println("Setup Done!");
  //DisplayCenterClear();
  Serial.println(WiFi.getHostname());
  //DisplayTimeoutReset(); //This allows the display to be shown for 10 seconds afer reboot. 
  //GPIOStart();
  pinMode(MP1INPUT, INPUT);
  // if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
  //   Serial.println("LittleFS Mount Failed");
  //   return;
  // }
  // listDir(LittleFS, "/", 1);
  // if(!LittleFS.exists("/hello3.txt")){
  //   writeFile(LittleFS, "/hello3.txt", "Hello3");
  // }
  // else if(!LittleFS.exists("/hello2.txt")){
  //   writeFile(LittleFS, "/hello2.txt", "Hello3");
  // }
  // else if(!LittleFS.exists("/hello1.txt")){
  //   writeFile(LittleFS, "/hello1.txt", "Hello3");
  // }
  // else{
  //   if(!LittleFS.exists("/hello.txt")){
  //   writeFile(LittleFS, "/hello.txt", "Hello");
  //   }
  // }
}

void loop() {
  //MqttLoop();
  //DisplayManager();
  //ScanUserInput();
  //SyncLoop();
  //WebHandel();
}



// void writeFile(fs::FS &fs, const char * path, const char * message){
//     Serial.printf("Writing file: %s\r\n", path);

//     File file = fs.open(path, FILE_WRITE);
//     if(!file){
//         Serial.println("- failed to open file for writing");
//         return;
//     }
//     if(file.print(message)){
//         Serial.println("- file written");
//     } else {
//         Serial.println("- write failed");
//     }
//     file.close();
// }

