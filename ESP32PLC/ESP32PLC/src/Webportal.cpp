#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "Sensors.h"
#include "Define.h"

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

#define LOG(f_, ...) \
  { Serial.printf((f_), ##__VA_ARGS__); }

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    wsconnected = true;
    LOG("ws[%s][%u] connect\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    wsconnected = false;
    LOG("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    LOG("ws[%s][%u] error(%u): %s\n", server->url(), client->id(),
        *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
    LOG("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len,
        (len) ? (char*)data : "");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      // the whole message is in a single frame and we got all of it's data
      LOG("ws[%s][%u] %s-msg[%llu]\r\n", server->url(), client->id(),
          (info->opcode == WS_TEXT) ? "txt" : "bin", info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char)data[i];
        }
        LOG("%s\r\n\r\n", msg.c_str());

        deserializeJson(jsonDocRx, msg);

        uint8_t ledState = jsonDocRx["led"];
        if (ledState == 1) {
          digitalWrite(LED, HIGH);
        }
        if (ledState == 0) {
          digitalWrite(LED, LOW);
        }
        jsonDocRx.clear();
      }
    }
  }
}

void WebStart(){
  /* Start web server and web socket server */
  lastButtonState = digitalRead(USER_SW);
  /* Start web server and web socket server */
  LOG("Web Service Start!\r");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.print("Root Page");
    //request->send(200, "text/html", "OK");
    request->send(SPIFFS, "/Main.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.print("got unhandled request for ");
    Serial.println(request->url());
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
      jsonDocTx["SSID"] = GetSSID();
      jsonDocTx["IP"] = GetIPStr();
      jsonDocTx["HN"] = GetHostName();
      jsonDocTx["RSSI"] = GetRSSIStr();
      jsonDocTx["MAC"] = GetMACStr();
      jsonDocTx["Temp"] = String(getDeviceClimateTemprature());
      jsonDocTx["Humid"] = String(getDeviceClimateHumidity());
      jsonDocTx["button"] = lastButtonState;
      jsonDocTx["Input1"] = lastButtonState;
      jsonDocTx["Input2"] = lastButtonState;
      jsonDocTx["Input3"] = lastButtonState;
      jsonDocTx["Input4"] = lastButtonState;
      jsonDocTx["Output1"] = lastButtonState;
      jsonDocTx["Output2"] = lastButtonState;
      jsonDocTx["Output3"] = lastButtonState;
      jsonDocTx["Output4"] = lastButtonState;

      serializeJson(jsonDocTx, output, 512);

      Serial.printf("Sending: %s", output);
      if (ws.availableForWriteAll()) {
        ws.textAll(output);
        Serial.printf("...done\r\n");
      } 
      else {
        Serial.printf("...queue is full\r\n");
      }
    }
  }
}
