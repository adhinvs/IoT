#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "base64.hpp"  // Include the base64 library

// WiFi Configuration
const char* ssid = "SSID";
const char* password = "PASS";

// Authentication Credentials (Salted and Hashed)
const char* www_username = "Admin";
const char* www_salt = "SecureRandomSalt123!"; // Add a unique salt
const char* www_password_hash = "49a8dae2a14a8fe64e83f9d53fd6323f217028ce7ffd9d8505ce11b978ed3199";
//Hash has be calculated seperately >> SHA256(USERNAMEPASSWORDSALT)

// LED Pin Configuration
const int ledPin = D4;  // Use D1 (GPIO5) for LED control

// Web Server Configuration
ESP8266WebServer server(80);

// Session Management
bool isLoggedIn = false;

// SHA256 Function (unchanged from previous code)
String calculateSHA256(String InputMessage) {
  // Initial hash values
  uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a,
           h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

  // Round constants
  uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
  };

  // Preprocessing
  uint64_t totalBits = InputMessage.length() * 8;
  InputMessage += (char)0x80;
  while (InputMessage.length() % 64 != 56) {
    InputMessage += (char)0x00;
  }
  
  // Append total bit length (big-endian)
  for (int i = 7; i >= 0; i--) {
    InputMessage += (char)((totalBits >> (i * 8)) & 0xFF);
  }

  // Process message in 64-byte blocks
  for (size_t chunk = 0; chunk < InputMessage.length(); chunk += 64) {
    uint32_t w[64] = {0};
    
    // Break chunk into 16 32-bit words
    for (int i = 0; i < 16; i++) {
      w[i] = ((uint32_t)InputMessage[chunk + i*4] << 24) |
             ((uint32_t)InputMessage[chunk + i*4 + 1] << 16) |
             ((uint32_t)InputMessage[chunk + i*4 + 2] << 8) |
             ((uint32_t)InputMessage[chunk + i*4 + 3]);
    }
    
    // Extend to 64 words
    for (int i = 16; i < 64; i++) {
      uint32_t s0 = ((w[i-15] >> 7) | (w[i-15] << 25)) ^ 
                    ((w[i-15] >> 18) | (w[i-15] << 14)) ^ 
                    (w[i-15] >> 3);
      uint32_t s1 = ((w[i-2] >> 17) | (w[i-2] << 15)) ^ 
                    ((w[i-2] >> 19) | (w[i-2] << 13)) ^ 
                    (w[i-2] >> 10);
      w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    
    // Initialize working variables
    uint32_t a = h0, b = h1, c = h2, d = h3, 
             e = h4, f = h5, g = h6, h = h7;
    
    // Main compression loop
    for (int i = 0; i < 64; i++) {
      uint32_t S1 = ((e >> 6) | (e << 26)) ^ 
                    ((e >> 11) | (e << 21)) ^ 
                    ((e >> 25) | (e << 7));
      uint32_t ch = (e & f) ^ ((~e) & g);
      uint32_t temp1 = h + S1 + ch + k[i] + w[i];
      uint32_t S0 = ((a >> 2) | (a << 30)) ^ 
                    ((a >> 13) | (a << 19)) ^ 
                    ((a >> 22) | (a << 10));
      uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      uint32_t temp2 = S0 + maj;
      
      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }
    
    // Update hash values
    h0 += a; h1 += b; h2 += c; h3 += d;
    h4 += e; h5 += f; h6 += g; h7 += h;
  }
  
  // Convert to hex string
  char hexBuffer[65];
  snprintf(hexBuffer, sizeof(hexBuffer), 
    "%08x%08x%08x%08x%08x%08x%08x%08x", 
    h0, h1, h2, h3, h4, h5, h6, h7);
  
  return String(hexBuffer);
}

// Compute Stored Password Hash
String computeStoredPasswordHash(const char* username, const char* password, const char* salt) {
  String combinedString = String(username) + password + salt;
  return calculateSHA256(combinedString);
}

// Login Page HTML
String getLoginPageHTML(String message = "") {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Login - Secure NodeMCU</title>"
                "<style>"
                "body {"
                "  font-family: Arial, sans-serif;"
                "  display: flex;"
                "  justify-content: center;"
                "  align-items: center;"
                "  height: 100vh;"
                "  margin: 0;"
                "  background-color: #f0f2f5;"
                "}"
                ".login-container {"
                "  background-color: white;"
                "  padding: 30px;"
                "  border-radius: 8px;"
                "  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);"
                "  width: 300px;"
                "}"
                "h1 {"
                "  text-align: center;"
                "  color: #333;"
                "}"
                "input {"
                "  width: 100%;"
                "  padding: 10px;"
                "  margin: 10px 0;"
                "  border: 1px solid #ddd;"
                "  border-radius: 4px;"
                "  box-sizing: border-box;"
                "}"
                ".button {"
                "  background-color: #4CAF50;"
                "  border: none;"
                "  color: white;"
                "  padding: 12px 20px;"
                "  text-align: center;"
                "  text-decoration: none;"
                "  display: inline-block;"
                "  width: 100%;"
                "  font-size: 16px;"
                "  margin: 10px 0;"
                "  cursor: pointer;"
                "  border-radius: 4px;"
                "}"
                ".error {"
                "  color: red;"
                "  text-align: center;"
                "  margin-bottom: 15px;"
                "}"
                "</style>"
                "</head>"
                "<body>"
                "<div class='login-container'>"
                "<h1>Secure Login</h1>";
  
  if (!message.isEmpty()) {
    html += "<div class='error'>" + message + "</div>";
  }
  
  html += "<form action='/login' method='POST'>"
          "<input type='text' name='username' placeholder='Username' required>"
          "<input type='password' name='password' placeholder='Password' required>"
          "<input type='submit' value='Login' class='button'>"
          "</form>"
          "</div>"
          "</body>"
          "</html>";
  
  return html;
}

// Login Handler
void handleLogin() {
  if (server.method() == HTTP_POST) {
    String submittedUsername = server.arg("username");
    String submittedPassword = server.arg("password");
    
    // Compute hash of submitted credentials
    String submittedHash = computeStoredPasswordHash(
      submittedUsername.c_str(), 
      submittedPassword.c_str(), 
      www_salt
    );

    // Compare credentials
    if (submittedUsername == www_username && 
        submittedHash == www_password_hash) {
      isLoggedIn = true;
      server.sendHeader("Location", "/dashboard");
      server.send(303);
    } else {
      server.send(200, "text/html", getLoginPageHTML("Invalid credentials. Please try again."));
    }
  } else {
    server.send(200, "text/html", getLoginPageHTML());
  }
}

// Logout Handler
void handleLogout() {
  isLoggedIn = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

// Dashboard Page Handler
void handleDashboard() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }

  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Dashboard - Secure NodeMCU</title>"
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
                ".logout {"
                "  position: absolute;"
                "  top: 20px;"
                "  right: 20px;"
                "  background-color: #f44336;"
                "}"
                "</style>"
                "</head>"
                "<body>"
                "<h1>NodeMCU Dashboard</h1>"
                "<a href='/logout' class='button logout'>Logout</a>"
                "<a href='/on' class='button'>Turn LED ON</a>"
                "<a href='/off' class='button'>Turn LED OFF</a>"
                "</body>"
                "</html>";
  
  server.send(200, "text/html", html);
}

// LED ON Handler
void handleLEDOn() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  digitalWrite(ledPin, HIGH);
  server.sendHeader("Location", "/dashboard");
  server.send(303);
}

// LED OFF Handler
void handleLEDOff() {
  if (!isLoggedIn) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/dashboard");
  server.send(303);
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
  
  // Define Web Server Routes
  server.on("/", handleLogin);
  server.on("/login", handleLogin);
  server.on("/logout", handleLogout);
  server.on("/dashboard", handleDashboard);
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
