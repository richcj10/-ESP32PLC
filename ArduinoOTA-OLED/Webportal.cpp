#include "Webportal.h"

WiFiServer server(80);

void WebStart(){
  server.begin();
}

// Variable to store the HTTP request
String header;

char* WebHandel(){
  WiFiClient client = server.available();
  if (client) {
    char sReturnCommand[32];
    int nCommandPos=-1;
    sReturnCommand[0] = '\0';
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if ((c == '\n') || (c == ' ' && nCommandPos>-1)) {
          sReturnCommand[nCommandPos] = '\0';
          if (strcmp(sReturnCommand, "\0") == 0) {
            httpResponseHome(client);
          } else {
            processCommand(sReturnCommand);
            httpResponseRedirect(client);
          }
          break;
        }
        if (nCommandPos>-1) {
          sReturnCommand[nCommandPos++] = c;
        }
        if (c == '?' && nCommandPos == -1) {
          nCommandPos = 0;
        }
      }
      if (nCommandPos > 30) {
        httpResponse414(client);
        sReturnCommand[0] = '\0';
        break;
      }
    }
    if (nCommandPos!=-1) {
      sReturnCommand[nCommandPos] = '\0';
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    
    return sReturnCommand;
  }
  return '\0';
}

/**
 * Command dispatcher
 */
void processCommand(char* command) {
  if        (strcmp(command, "1-on") == 0) {
    //mySwitch.switchOn(1,1);
    Serial.println("1 ON");
  } else if (strcmp(command, "1-off") == 0) {
    //mySwitch.switchOff(1,1);
    Serial.println("1 OFF");
  } else if (strcmp(command, "2-on") == 0) {
    //mySwitch.switchOn(1,2);
    Serial.println("2 ON");
  } else if (strcmp(command, "2-off") == 0) {
    //mySwitch.switchOff(1,2);
    Serial.println("2 OFF");
  }
}

/**
 * HTTP Response with homepage
 */
void httpResponseHome(WiFiClient c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: text/html");
  c.println();
  c.println("<html>");
  c.println("<head>");
  c.println(    "<title>RCSwitch Webserver Demo</title>");
  c.println(    "<style>");
  c.println(        "body { font-family: Arial, sans-serif; font-size:12px; }");
  c.println(    "</style>");
  c.println("</head>");
  c.println("<body>");
  c.println(    "<h1>RCSwitch Webserver Demo</h1>");
  c.println(    "<ul>");
  c.println(        "<li><a href=\"./?1-on\">Switch #1 on</a></li>");
  c.println(        "<li><a href=\"./?1-off\">Switch #1 off</a></li>");
  c.println(    "</ul>");
  c.println(    "<ul>");
  c.println(        "<li><a href=\"./?2-on\">Switch #2 on</a></li>");
  c.println(        "<li><a href=\"./?2-off\">Switch #2 off</a></li>");
  c.println(    "</ul>");
  c.println(    "<hr>");
  c.println(    "<a href=\"https://github.com/sui77/rc-switch/\">https://github.com/sui77/rc-switch/</a>");
  c.println("</body>");
  c.println("</html>");
}

/**
 * HTTP Redirect to homepage
 */
void httpResponseRedirect(WiFiClient c) {
  c.println("HTTP/1.1 301 Found");
  c.println("Location: /");
  c.println();
}

/**
 * HTTP Response 414 error
 * Command must not be longer than 30 characters
 **/
void httpResponse414(WiFiClient c) {
  c.println("HTTP/1.1 414 Request URI too long");
  c.println("Content-Type: text/plain");
  c.println();
  c.println("414 Request URI too long");
}
