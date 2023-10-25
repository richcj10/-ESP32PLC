#include <ArduinoJson.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "Sensors.h"
#include "Define.h"

#include "Devices/Log.h"

#define HTTP_PORT 80

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

StaticJsonDocument<200> jsonDocTx;
StaticJsonDocument<100> jsonDocRx;

bool wsconnected = false;
bool lastButtonState = 0;
static char output[512];

unsigned long cnt = 0;
unsigned long LastTime = 0;

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    wsconnected = true;
    Log(NOTIFY,"ws[%s][%u] connect\r\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    wsconnected = false;
    Log(NOTIFY,"ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    Log(ERROR,"WS Error");
  } else if (type == WS_EVT_PONG) {
    Log(NOTIFY,"WS pong");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      // the whole message is in a single frame and we got all of it's data
      Log(NOTIFY,"ws[%s][%u] %s-msg[%llu]\r\n", server->url(), client->id(),
          (info->opcode == WS_TEXT) ? "txt" : "bin", info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char)data[i];
        }
        Log(NOTIFY,"%s\r\n\r\n", msg.c_str());

        deserializeJson(jsonDocRx, msg);
        jsonDocRx.clear();
      }
    }
  }
}

void WebStart(){
  /* Start web server and web socket server */
  //lastButtonState = digitalRead(USER_SW);
  /* Start web server and web socket server */
  Log(DEBUG,"Web Server Start!");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Log(DEBUG,"MP server\r\n");
    //request->send(200, "text/html", "OK");
    request->send(LittleFS, "/Main.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");
  server.onNotFound([](AsyncWebServerRequest *request){
    Log(DEBUG,"Bad Request: %s\r\n",request->url());
    request->send(404);
  });
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
}

void WebHandel(){
  if((millis() - LastTime) > 2000){
    LastTime = millis();
    if(wsconnected == true){
      lastButtonState = digitalRead(USER_SW);
      jsonDocTx.clear();
      //jsonDocTx["SSID"] = GetSSID();
      //jsonDocTx["IP"] = GetIPStr();
      //jsonDocTx["HN"] = GetHostName();
      //jsonDocTx["RSSI"] = GetRSSIStr();
      //jsonDocTx["MAC"] = GetMACStr();
      ///jsonDocTx["Temp"] = String(getDeviceClimateTemprature());
      //jsonDocTx["Humid"] = String(getDeviceClimateHumidity());
      //jsonDocTx["button"] = lastButtonState;
      //jsonDocTx["Input1"] = lastButtonState;
      //jsonDocTx["Input2"] = lastButtonState;
      //jsonDocTx["Input3"] = lastButtonState;
      //jsonDocTx["Input4"] = lastButtonState;
      //jsonDocTx["Output1"] = lastButtonState;
      //jsonDocTx["Output2"] = lastButtonState;
      //jsonDocTx["Output3"] = lastButtonState;
      //jsonDocTx["Output4"] = lastButtonState;

      serializeJson(jsonDocTx, output, 512);

      Log(DEBUG,"Sending Data\r\n");
      if (ws.availableForWriteAll()) {
        ws.textAll(output);
        Log(DEBUG,"Data Sent\r\n");
      } 
      else {
        Log(DEBUG,"Data que\r\n");
      }
    }
  }
}

char WebLogSend(String LogString){
  if(wsconnected == true){
    //Serial.println(LogString);
    jsonDocTx.clear();
    jsonDocTx["Type"] = 10; //Log Send Command
    jsonDocTx["LOG"] = LogString + "\n";//\n";
    serializeJson(jsonDocTx, output, 512);
    if (ws.availableForWriteAll()) {
      ws.textAll(output);
        //Log(NOTIFY,"Sent Log");
    } 
/*       else {
        Log(ERROR,"Queue Is Full");
      } */
    return 1;
  }
  return 0;
}
