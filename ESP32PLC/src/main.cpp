#include "Functions.h"
#include "Display/Oled.h"
#include "MQTT.h"
#include "Sensors.h"

#include "Display/Display.h"
#include "HAL/DeviceConfig.h"
#include "FileSystem/FSInterface.h"
#include "WifiControl/WifiConfig.h"
#include "HAL/Digital/Digital.h"
#include "HAL/Com/I2C.h"
#include "Devices/StatusLED.h"
#include "Devices/JoyStick.h"
#include "Webportal.h"
#include "Devices/Log.h"
#include "Remote/MasterController.h"

#include "Display/TFT.h"

// Start ArduinoOTA via WiFiSettings with the same hostname and password
int BuffSize = 0;

void setup() {
  SystemStart();
  Log(NOTIFY,"Startup Complete");
  pinMode(2, OUTPUT);
  pinMode(16,INPUT);
  pinMode(40,OUTPUT); // Relay Control 
  pinMode(41,OUTPUT); // Relay Control, Lights
  pinMode(42,OUTPUT); // Relay Control, Lights
  pinMode(39,OUTPUT); // Relay Control, Lights
  LEDBoot();
  if(FileStstemStart()){
    DisplayLog(" FS OK ");
    delay(1000);
  }
  else{
    DisplayLog(" FS ERROR ");
    delay(1000);
  }
  QueryLocalDevice();


  //GPIOStart();
  //setupMode();
  //Serial.print("SiSensor = ");
  //Serial.println(Si7021checkID());
  DisplayLog(" Connecting to WiFi...");
  if(!SetupWiFi()){
    Serial.println(" WiFi Fialed");
    DisplayLog("Connection Failed..Setting up AP");
    SetWiFisetupMode(WIFI_AP_MODE);
    SetupWiFi();
  }
  setup_ota();
  DisplayLog(GetIPStr().c_str());
  delay(1000);
  InitSensors();
  MQTTStart();
  WebStart();
  //DisplayCenterClear();
  //GPIOStart();
  //pinMode(MP1INPUT, INPUT);
  //I2CScan();
  SetLEDStatus(NORMAL,1000);
  Serial.println("Setup Done!");
  DisplayTimeoutReset();//This allows the display to be shown for 10 seconds afer reboot.
  TFTBargraph(1);
  pinMode(42, OUTPUT);
  pinMode(41, OUTPUT);
  pinMode(40, OUTPUT);
  pinMode(39, OUTPUT);
  pinMode(38, OUTPUT);
}

int l =0;
unsigned long LastSendTime, LastSendTime2, LastSendTime3 = 0;
char TSChannel = 1;
char Send,Prect = 0;

unsigned long LastSendTimeCH1, LastSendTimeCH2, LastSendTimeCH3, LastSendTimeCH4, LastSendTimeCH5 =0;
char CH1FT, CH2FT, CH3FT, CH4FT, CH5FT = 0;

void loop() {
  //
  
  RemoteRun();
  
  
  UIUpdateLoop();
  SensorUpdateLoop();
  
  //
  //Serial.println("loop");
  //GetJoystickPrint(GetJoyStickPos());
  MqttLoop();
  
  //ScanUserInput();
  SyncLoop();
/*   if(millis() - LastSendTime > 3000){
    LastSendTime = millis();
    //SetCHFire(4, 1);
    //ReadRemoteCurrent();
    //ReadRemoteTemp();
    //GetRemoteTemp(10);
  } */
  
  if(millis() - LastSendTime2 > 1500){
    LastSendTime2 = millis();
    if(Send == 0) SendOutsideEnvoroment();
    if(Send == 1) SendRemoteRTD();
    if(Send == 2) SendRemoteCurrentSense();
    Send++;
    if(Send > 3){
      Send = 0;
    }
    //ReadRemoteWeather();
  }

  if(millis() - LastSendTime3 > 200){
    LastSendTime3 = millis();
    Prect++;
    if(Prect > 100){
      //TFTDisplayClear();
      //TFTBargraph(1);
      Prect = 0;
    }
    //TFTBargraphUpdate(Prect);
  }
  //DisplayWiFiSignal();
  //SetOcupyLED(0x11,200,0,0);
  //SetOcupyLED(0x10,0,0,0);
  //delay(1000);
  //SetOcupyLED(0x11,0,200,0);
  //SetOcupyLED(0x10,0,0,200);
  delay(1);
  //Serial.println(readDeviceVIN(0x10));
  //Serial.println(readDeviceVIN(0x11));
  //Serial.println(readDeviceVIN(0x20));
  //RunFW();

/*   if(GetCHFire(1) == 1){
    if(CH1FT == 0){
      digitalWrite(40,HIGH);  //CH1 
      LastSendTimeCH1 = millis();
      CH1FT = 1;
    }
    if(millis() - LastSendTimeCH1 > 2500){
      SetCHFire(1, 0);
      digitalWrite(40,LOW);  //CH1 
      CH1FT = 0;
    }
  }
  if(GetCHFire(2) == 1){
    if(CH2FT == 0){
      digitalWrite(41,HIGH);  //CH1 
      LastSendTimeCH2 = millis();
      CH2FT = 1;
    }
    if(millis() - LastSendTimeCH2 > 2500){
      SetCHFire(2, 0);
      digitalWrite(41,LOW);  //CH1 
      CH2FT = 0;
    }
  }
  if(GetCHFire(3) == 1){
    if(CH3FT == 0){
      digitalWrite(39,HIGH);  //CH1 
      LastSendTimeCH3 = millis();
      CH3FT = 1;
    }
    if(millis() - LastSendTimeCH3 > 2500){
      SetCHFire(3, 0);
      digitalWrite(39,LOW);  //CH1 
      CH3FT = 0;
    }
  }
  if(GetCHFire(4) == 1){
    if(CH4FT == 0){
      digitalWrite(42,HIGH);  //CH1 
      LastSendTimeCH4 = millis();
      CH4FT = 1;
    }
    if(millis() - LastSendTimeCH4 > 2500){
      SetCHFire(4, 0);
      digitalWrite(42,LOW);  //CH1 
      CH4FT = 0;
    }
  }
  if(GetCHFire(5) == 1){
    if(CH5FT == 0){
      digitalWrite(38,HIGH);  //CH1 
      LastSendTimeCH5 = millis();
      CH5FT = 1;
    }
    if(millis() - LastSendTimeCH5 > 2500){
      SetCHFire(5, 0);
      digitalWrite(38,LOW);  //CH1 
      CH5FT = 0;
    }
  } */
}

