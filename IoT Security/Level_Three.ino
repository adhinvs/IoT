#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi Configuration
const char* ssid = "WIFI";
const char* password = "PASS";

// Web Server Configuration
ESP8266WebServer server(80);

// LED Pin Configuration
const int LED_PIN = D4;  // Built-in LED on NodeMCU

// Authentication Flag
bool isLoggedIn = false;

// Base64 Decoding Function (unchanged from previous version)
String base64Decode(const String& input) {
  const String base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String decoded = "";
  int in_len = input.length();
  int i = 0, j = 0, in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  while (in_len-- && (input[in_] != '=')) {
    char_array_4[i++] = input[in_]; in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.indexOf(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; i < 3; i++)
        decoded += char(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] = base64_chars.indexOf(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; j < i - 1; j++) 
      decoded += char(char_array_3[j]);
  }

  return decoded;
}

// Serve HTML Page (with modified JavaScript)
void serveHomePage() {
  String html = F(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "  <title>NodeMCU LED Control</title>"
    "  <style>"
    "    body { font-family: Arial, sans-serif; text-align: center; }"
    "    .container { width: 300px; margin: 50px auto; }"
    "    input { margin: 10px 0; padding: 5px; width: 100%; }"
    "    button { padding: 10px; width: 100%; margin: 5px 0; }"
    "  </style>"
    "</head>"
    "<body>"
    "  <div class='container'>"
    "    <h2>NodeMCU Login</h2>"
    "    <div id='loginForm'>"
    "      <input type='text' id='username' placeholder='Username'>"
    "      <input type='password' id='password' placeholder='Password'>"
    "      <button onclick='login()'>Login</button>"
    "    </div>"
    "    <div id='controlPanel' style='display:none;'>"
    "      <h3>LED Control</h3>"
    "      <button onclick='toggleLED(\"on\")'>Turn ON</button>"
    "      <button onclick='toggleLED(\"off\")'>Turn OFF</button>"
    "      <br><br>"
    "      <button onclick='logout()'>Logout</button>"
    "    </div>"
    "  </div>"
    "  <script>"
    "    function login() {"
    "      var username = document.getElementById('username').value;"
    "      var password = document.getElementById('password').value;"
    "      var encryptedCreds = btoa(username + ':' + password);"
    "      console.log('Sending credentials:', encryptedCreds);"
    ""
    "      fetch('/login', {"
    "        method: 'POST',"
    "        headers: {"
    "          'Content-Type': 'application/x-www-form-urlencoded'"
    "        },"
    "        body: 'credentials=' + encodeURIComponent(encryptedCreds)"
    "      })"
    "      .then(response => {"
    "        console.log('Response status:', response.status);"
    "        return response.json();"
    "      })"
    "      .then(data => {"
    "        console.log('Response data:', data);"
    "        if (data.success) {"
    "          document.getElementById('loginForm').style.display = 'none';"
    "          document.getElementById('controlPanel').style.display = 'block';"
    "        } else {"
    "          alert('Login Failed: ' + (data.error || 'Unknown error'));"
    "        }"
    "      })"
    "      .catch(error => {"
    "        console.error('Error:', error);"
    "        alert('Login Error');"
    "      });"
    "    }"
    ""
    "    function toggleLED(state) {"
    "      fetch('/led?state=' + state)"
    "      .then(response => {"
    "        if (response.status === 401) {"
    "          alert('Unauthorized. Please login.');"
    "          return null;"
    "        }"
    "        return response.json();"
    "      })"
    "      .then(data => {"
    "        if (data && !data.success) {"
    "          alert('Failed to change LED state');"
    "        }"
    "      });"
    "    }"
    ""
    "    function logout() {"
    "      fetch('/logout')"
    "      .then(response => response.json())"
    "      .then(data => {"
    "        if (data.success) {"
    "          document.getElementById('loginForm').style.display = 'block';"
    "          document.getElementById('controlPanel').style.display = 'none';"
    "        }"
    "      });"
    "    }"
    "  </script>"
    "</body>"
    "</html>"
  );
  
  server.send(200, "text/html", html);
}

// Simplified authentication check
bool isAuthenticated() {
  return isLoggedIn;
}

void setup() {
  Serial.begin(115200);
  
  // LED Setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED is active LOW on NodeMCU

  // WiFi Connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  // Web Server Routes
  server.on("/", HTTP_GET, serveHomePage);

  server.on("/login", HTTP_POST, []() {
    Serial.println("Login Attempt Received");
    
    if (server.hasArg("credentials")) {
      String credentials = server.arg("credentials");
      
      // Debug: Print received credentials
      Serial.println("Received Credentials: " + credentials);
      
      // Decode base64
      String decodedCreds = base64Decode(credentials);
      
      // Debug: Print decoded credentials
      Serial.println("Decoded Credentials: " + decodedCreds);
      
      int colonIndex = decodedCreds.indexOf(':');
      
      // Debug: Check if colon is found
      if (colonIndex == -1) {
        Serial.println("No colon found in decoded credentials");
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid credentials format\"}");
        return;
      }
      
      String username = decodedCreds.substring(0, colonIndex);
      String pwd = decodedCreds.substring(colonIndex + 1);
      
      // Debug: Print parsed username and password
      Serial.println("Username: " + username);
      Serial.println("Password: " + pwd);

      if (username == "admin" && pwd == "password") {
        Serial.println("Login Successful");
        isLoggedIn = true;  // Set authentication flag
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        Serial.println("Login Failed");
        isLoggedIn = false;
        server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid credentials\"}");
      }
    } else {
      Serial.println("No credentials received");
      isLoggedIn = false;
      server.send(400, "application/json", "{\"success\":false,\"error\":\"No credentials\"}");
    }
  });

  server.on("/led", HTTP_GET, []() {
    // Check authentication using the simplified method
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      return;
    }

    if (server.hasArg("state")) {
      String state = server.arg("state");
      if (state == "on") {
        digitalWrite(LED_PIN, LOW);  // Turn ON (active LOW)
        server.send(200, "application/json", "{\"success\":true}");
      } else if (state == "off") {
        digitalWrite(LED_PIN, HIGH);  // Turn OFF
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"success\":false}");
      }
    }
  });

  server.on("/logout", HTTP_GET, []() {
    isLoggedIn = false;  // Reset authentication flag
    server.send(200, "application/json", "{\"success\":true}");
  });

  server.begin();
  Serial.println("HTTP Server Started");
}

void loop() {
  server.handleClient();
}
