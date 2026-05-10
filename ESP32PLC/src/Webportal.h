#ifndef WEBPORTAL_H
#define WEBPORTAL_H

#include <WiFi.h>

void WebStart();
void WebHandel();
void CaptivePortalLoop();

void httpResponseRedirect(WiFiClient c);
void httpResponseHome(WiFiClient c);
void processCommand(char* command);
void httpResponse414(WiFiClient c);
char WebLogSend(String LogString);

/* Send a firmware-update progress event over WebSocket (Type=20).
 * Call from any task — ws.textAll() is thread-safe in ESPAsyncWebServer.
 * done=true ends the session; success only meaningful when done=true.
 * msg may be nullptr.                                                    */
void WebFwProgressSend(uint8_t percent, bool done, bool success, const char *msg);

#endif  /* WEBPORTAL_H */
