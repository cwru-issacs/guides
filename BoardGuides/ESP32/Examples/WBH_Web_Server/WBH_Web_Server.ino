#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// Set your network credentials here
const char* ssid = "CaseGuest";
const char* password = "";

// Set the name of the device here
const char* host_name = "esp32";

WebServer server(80);

const int led = 2;

void setup(void) {
  // Configure Status LED
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  // Start Serial Monitor
  Serial.begin(115200);

  // Connect to Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(led, !digitalRead(led)); // Toggle LED while waiting
  }
  Serial.println("");
  digitalWrite(led, 0);

  // Print Connected Message (with IP Address) to Serial Monitor
  Serial.print(F("Connected to "));
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Register MDNS name
  if (MDNS.begin(host_name)) {
    Serial.print(F("MDNS responder started with Hostname: "));
    Serial.print(host_name);
    Serial.println(F(".local"));
  }

  // Setup Callback Functions for Web URLs
  server.on("/", handleRoot);
  server.on("/wbh.css", handleWbhCss);
  server.onNotFound(handleNotFound);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  // Start Web Server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}


//------------------------------------------------------------------
// Web Page Handler Routines
//------------------------------------------------------------------

void handleRoot() {
  static unsigned long pageloads = 0;
  digitalWrite(led, 1);
  String message = F("<!DOCTYPE html>\n"
                   "<html>"
                   "<head>"
                   "<meta charset=\"UTF-8\">"
                   "<title>ESP32 WebServer Demo</title>"
                   "<style>"
                   "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }"
                   "</style>"
                   "<link rel=\"stylesheet\" href=\"wbh.css\">"
                   "</head>"
                   "<body>"
                   "<div class=\"topheader\">"
                   "<span class=\"topleft\">"
                   "<h1>think[box] WelcomeBackHack ESP32 Web Demo</h1>"
                   "</span>"
                   "<span class=\"topright\">"
                   "<h1>Page Loads: {0}</h1>"
                   "</span>​"
                   "</div>"
                   "<center>"
                   "<img src=\"https://cwru-issacs.github.io/wbh19/images/WBH_small.png\" alt=\"Welcome Back Hack 2019\">"
                   "</center>"
                   "</body>"
                   "</html>\n");
  message.replace("{0}", String(++pageloads));
  server.send(200, "text/html", message);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handleWbhCss() {
  digitalWrite(led, 1);
  String message = F(".topheader {\n"
                   "  background-color: #308DE5;\n"
                   "  color: #DCE8F4;\n"
                   "  padding: 10px;\n"
                   "}\n"
                   ".topright{\n"
                   "  float: right;\n"
                   "  display: inline-block;\n"
                   "}\n"
                   ".topleft{\n"
                   "  float: center;\n"
                   "  display: inline-block;\n"
                   "}\n");
  server.send(200, "text/css", message);
  digitalWrite(led, 0);
}
