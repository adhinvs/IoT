#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi Configuration
const char* ssid = "SSID";
const char* password = "PASS";

// LED Pin Configuration
const int ledPin = D1;  // Use D1 (GPIO5) for LED control

// Web Server Configuration
ESP8266WebServer server(80);

void setup() {
  // Initialize Serial Communication
  Serial.begin(115200);
  
  // Configure LED Pin as Output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Ensure LED is off initially

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Define Web Server Routes
  server.on("/", handleRoot);
  server.on("/on", handleLEDOn);
  server.on("/off", handleLEDOff);
  
  // Start Web Server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle Client Requests
  server.handleClient();
}

// Root Page Handler
void handleRoot() {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>NodeMCU LED Control</title>"
                "<style>"
                "body { font-family: Arial; text-align: center; }"
                ".button { "
                "  background-color: #4CAF50; "
                "  border: none; "
                "  color: white; "
                "  padding: 15px 32px; "
                "  text-align: center; "
                "  text-decoration: none; "
                "  display: inline-block; "
                "  font-size: 16px; "
                "  margin: 10px; "
                "  cursor: pointer; "
                "}"
                "</style>"
                "</head>"
                "<body>"
                "<h1>NodeMCU LED Control</h1>"
                "<a href='/on' class='button'>Turn ON</a>"
                "<a href='/off' class='button'>Turn OFF</a>"
                "</body>"
                "</html>";
  
  server.send(200, "text/html", html);
}

// LED ON Handler
void handleLEDOn() {
  digitalWrite(ledPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

// LED OFF Handler
void handleLEDOff() {
  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}
