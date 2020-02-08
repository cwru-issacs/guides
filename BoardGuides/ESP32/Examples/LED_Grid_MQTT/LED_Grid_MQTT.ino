
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <esp32_digital_led_lib.h>
#include <esp32_digital_led_funcs.h>
#include "issacs_hackcwru_banner.h"

// Set your network credentials here
const char* ssid = "CaseGuest";
const char* password = "";

// Set your MQTT Broker IP address
const char* mqtt_server = "172.19.39.121";

// Set the name of the device here
const char* host_name = "issacs-esp32";

WebServer server(80);

WiFiClient espClient;
PubSubClient client(espClient);


const int led_pin = 2;

// Set the LED Grid Configuration
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"  // It's noisy here with `-Wall`

strand_t strand = {.rmtChannel = 0, .gpioNum = 5, .ledType = LED_WS2812B_V3, .brightLimit = 64, .numPixels = 200};
strand_t * STRANDS [] = { &strand };
int STRANDCNT = COUNT_OF(STRANDS); 

strand_t mqtt_strand = {.rmtChannel = 0, .gpioNum = 5, .ledType = LED_WS2812B_V3, .brightLimit = 64, .numPixels = 200};
strand_t * MQTT_STRANDS [] = { &mqtt_strand };
int MQTT_STRANDCNT = COUNT_OF(MQTT_STRANDS); 


#pragma GCC diagnostic pop

#define GRID_X 8
#define GRID_Y 25

void setGridPixelRGB(strand_t &strand, uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {

  int pixel_index;
  
  if ( (x % 2) == 0 )
    pixel_index = x * GRID_Y + y;
  else
    pixel_index = x * GRID_Y + (GRID_Y-1) - y;

  // Verify x/y position is on LED strip
  if (pixel_index >= strand.numPixels) {
    Serial.print("Bad pixel Index"); Serial.println(pixel_index);
    return;
  }

  // Set the pixel color
  strand.pixels[pixel_index] = pixelFromRGB(r, g, b);

  return;  
}

volatile int subscriber_count = 0;
volatile int banner_select = 0;
volatile int current_row = 0;
volatile bool auto_scroll = true;

//**************************************************************************//
void setup()
{
  // Start the Serial Monitor
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Configure Status LED
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, 0);

  // Initialize the LED Strands
  digitalLeds_initDriver();

  gpioSetup(strand.gpioNum, OUTPUT, LOW);
  int rc = digitalLeds_addStrands(STRANDS, STRANDCNT);
  if (rc) {
    Serial.print("Init rc = ");
    Serial.println(rc);
  }

  if (digitalLeds_initDriver()) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  digitalLeds_resetPixels(STRANDS, STRANDCNT);

  // Connect to Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(led_pin, !digitalRead(led_pin)); // Toggle LED while waiting
  }
  Serial.println("");
  digitalWrite(led_pin, 0);

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

  // Setup Callback Functions for MQTT messages
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

  // Setup Callback Functions for Web URLs
  server.on("/", handleRoot);
  server.on("/wbh.css", handleWbhCss);
  server.onNotFound(handleNotFound);
  server.on("/subscribe", []() {
    subscriber_count++;
    server.send(200, "text/plain", "Welcome!!!\n");
  });
  server.on("/unsubscribe", []() {
    subscriber_count--;
    server.send(200, "text/plain", "Goodbye.\n");
  });
  server.on("/banner", []() {
    banner_select = 0;
    server.send(200, "text/plain", "changed display to issacs banner\n");
  });
  server.on("/mqttdemo", []() {
    banner_select = 1;
    server.send(200, "text/plain", "changed display to mqtt demo\n");
  });
  server.on("/disco", []() {
    banner_select = 2;
    server.send(200, "text/plain", "changed display to disco banner\n");
  });

  // Start Web Server
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Init complete");
}

void mqtt_callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  uint8_t msg[5];
  char msg_label[5] = {'x', 'y', 'r', 'g', 'b'}; 
  
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(led_pin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(led_pin, LOW);
    }
  } else if (String(topic) == "esp32/pixels/setcolor") {
    char *p;
    p = strtok((char*)messageTemp.c_str(), ",");

    for (int i = 0; i < 5; i++) {
      if (p != NULL) {
        msg[i] = (uint8_t)strtol(p, NULL, 16);
        Serial.print (msg_label[i]);
        Serial.print (" = ");
        Serial.println (msg[i]);
      } else {
        Serial.println ("Bad Message");
        break;
      }
      p = strtok(NULL, ",");
      
      if (i == 4) {
        if (msg[0] == 255 && msg[1] == 255) {
          for (int x = 0; x < GRID_X; x++) {
            for (int y = 0; y < GRID_Y; y++) {
              setGridPixelRGB(strand, x, y, msg[2], msg[3], msg[4]);
            }
          }
        } else {
          setGridPixelRGB(strand, msg[0], msg[1], msg[2], msg[3], msg[4]);
        }
      }
    }  
  }
}

void mqtt_reconnect() {
  static unsigned long last_update_time = 0;
  unsigned long current_time;

  // Wait 5 seconds before retrying
  current_time = millis();
  if (current_time - last_update_time > 5000) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("esp32/pixels/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
    last_update_time = current_time;   
  }
}

//------------------------------------------------------------------
// LED Grid Routines
//------------------------------------------------------------------

void displayBanner(strand_t &strand, const uint8_t * banner_data, uint8_t start_row, uint8_t max_row) {

  uint32_t dimmer = 0x001F1F1F;

  if (start_row >= max_row)
    return;
  
  // Write Solid Background
  for (int x = 0; x < GRID_X; x++) {
    for (int y = 0; y < GRID_Y; y++) {
      setGridPixelRGB(strand, x, y, 0, 0, 42); // Blue
    }
  }
  
  // Write Banner    
  for (int y = 0; y < GRID_Y; y++) {
    uint8_t b = 0x80;
    for (int x = 0; x < GRID_X; x++) {
      if (b & banner_data[(start_row + y)%max_row]) {
        setGridPixelRGB(strand, x, y, 64, 64, 64);
      }
      b >>= 1;
    }
  }

  // Update the display
  digitalLeds_drawPixels(STRANDS, STRANDCNT);
}

void displayRandom(strand_t &strand) {
  
  uint32_t dimmer = 0x001F1F1F;

  // Random Background
  for (uint16_t i = 0; i < strand.numPixels; i++) {
    strand.pixels[i].raw32 = (esp_random() & dimmer);
  }
  digitalLeds_drawPixels(STRANDS, STRANDCNT);
}

//**************************************************************************//

#define SCAN_RATE 100

void loop()
{
  static unsigned long last_update_time = 0;
  unsigned long current_time;
  
  // Handle Webserver connections
  server.handleClient();

  // Handle MQTT connections
  if (!client.connected()) {
    mqtt_reconnect();
  } else {
    client.loop();
  }
  
  // Update the display
  current_time = millis();
  if (current_time - last_update_time > SCAN_RATE) {
    if (auto_scroll)
      current_row++;
      
    // Display the selected banner
    if (banner_select == 0) {
      if (current_row >= ISSACS_HACKCWRU_BANNER_ROWS || current_row < 0) current_row = 0;  // Race condition here!
      displayBanner(strand, issacs_hackcwru_banner_bitmap, current_row, ISSACS_HACKCWRU_BANNER_ROWS);
    } else if (banner_select == 1) {
        digitalLeds_drawPixels(STRANDS, STRANDCNT);
    } else {
      displayRandom(strand);
    }

    last_update_time = current_time;
  }
}



//------------------------------------------------------------------
// Web Page Handler Routines
//------------------------------------------------------------------

void handleRoot() {
  static unsigned long pageloads = 0;
  digitalWrite(led_pin, 1);
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
  digitalWrite(led_pin, 0);
}

void handleNotFound() {
  digitalWrite(led_pin, 1);
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
  digitalWrite(led_pin, 0);
}

void handleWbhCss() {
  digitalWrite(led_pin, 1);
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
  digitalWrite(led_pin, 0);
}
