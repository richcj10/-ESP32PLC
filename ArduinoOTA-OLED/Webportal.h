#ifndef WEBPORTAL_H
#define  WEBPORTAL_H

#include <WiFi.h>

void WebStart();
char* WebHandel();

void httpResponseRedirect(WiFiClient c);
void httpResponseHome(WiFiClient c);
void processCommand(char* command);
void httpResponse414(WiFiClient c);

#endif  /* SENSORS_H */
