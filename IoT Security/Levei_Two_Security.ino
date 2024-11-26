#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Authentication Credentials
const char* www_username = "admin";
const char* www_password = "yourSecurePassword123";

// LED Pin Configuration
const int ledPin = D1;  // Use D1 (GPIO5) for LED control

// Web Server Configuration
ESP8266WebServer server(80);

// Authentication Function
bool isAuthenticated() {
  // Check if Authorization header exists
  if (!server.hasHeader("Authorization")) return false;
  
  // Get Authorization header
  String authHeader = server.header("Authorization");
  
  // Direct string comparison
  return (authHeader == String("Basic ") + 
          base64::encode(String(www_username) + ":" + www_password));
}

// Authentication Prompt
void sendAuthFailResponse() {
  server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
  server.send(401, "text/plain", "Authentication Failed");
}

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

  // Define Web Server Routes with Authentication
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

// Root Page Handler with Authentication
void handleRoot() {
  // Check Authentication
  if (!isAuthenticated()) {
    sendAuthFailResponse();
    return;
  }

  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Secure NodeMCU LED Control</title>"
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
                "<h1>Secure NodeMCU LED Control</h1>"
                "<a href='/on' class='button'>Turn ON</a>"
                "<a href='/off' class='button'>Turn OFF</a>"
                "</body>"
                "</html>";
  
  server.send(200, "text/html", html);
}

// LED ON Handler with Authentication
void handleLEDOn() {
  // Check Authentication
  if (!isAuthenticated()) {
    sendAuthFailResponse();
    return;
  }

  digitalWrite(ledPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

// LED OFF Handler with Authentication
void handleLEDOff() {
  // Check Authentication
  if (!isAuthenticated()) {
    sendAuthFailResponse();
    return;
  }

  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}
