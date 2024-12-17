#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AESLib.h>

// WiFi Configuration
const char* ssid = "WIFI";
const char* password = "PASS";

// Web Server Configuration
ESP8266WebServer server(80);

// LED Pin Configuration
const int LED_PIN = D4;  // Built-in LED on NodeMCU

// Encryption Configuration
AESLib aesLib;
uint8_t key[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                  0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };

// Authentication Flag
bool isLoggedIn = false;

// AES Decryption Function
String decryptMessage(const String& encryptedMessage, const String& ivHex) {
  int ivLen = ivHex.length() / 2;
  uint8_t iv[ivLen];
  for (int i = 0; i < ivLen; i++) {
    iv[i] = strtoul(ivHex.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
  }

  int len = encryptedMessage.length() / 2;
  uint8_t* encrypted = new uint8_t[len];
  for (int i = 0; i < len; i++) {
    encrypted[i] = strtoul(encryptedMessage.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
  }

  uint8_t* decrypted = new uint8_t[len];
  int decryptedLength = aesLib.decrypt(encrypted, len, decrypted, key, 16, iv);

  // Remove PKCS padding bytes
  if (decryptedLength > 0) {
    while (decryptedLength > 0 && decrypted[decryptedLength - 1] <= 16) {
      decryptedLength--;
    }
  }

  String result = "";
  for (int i = 0; i < decryptedLength; i++) {
    result += (char)decrypted[i];
  }

  // Debug logs
  Serial.println("Decrypted Message: " + result);

  delete[] encrypted;
  delete[] decrypted;

  return result;
}

// Serve HTML Page
void serveHomePage() {
  String html = F(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "  <title>NodeMCU LED Control</title>"
    "  <script src=\"https://cdnjs.cloudflare.com/ajax/libs/crypto-js/4.1.1/crypto-js.min.js\"></script>"
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
    "    const key = CryptoJS.enc.Hex.parse('0123456789ABCDEFFEDCBA9876543210');"
    ""
    "    function generateIV() {"
    "      return CryptoJS.lib.WordArray.random(16).toString(CryptoJS.enc.Hex);"
    "    }"
    ""
    "    function login() {"
    "      const username = document.getElementById('username').value;"
    "      const password = document.getElementById('password').value;"
    "      const credentials = username + ':' + password;"
    ""
    "      const iv = generateIV();"
    "      const encrypted = CryptoJS.AES.encrypt(credentials, key, { iv: CryptoJS.enc.Hex.parse(iv) }).ciphertext.toString(CryptoJS.enc.Hex);"
    ""
    "      fetch('/login', {"
    "        method: 'POST',"
    "        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
    "        body: 'iv=' + encodeURIComponent(iv) + '&credentials=' + encodeURIComponent(encrypted)"
    "      })"
    "      .then(response => response.json())"
    "      .then(data => {"
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
    "      .then(response => response.json())"
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

bool isAuthenticated() {
  return isLoggedIn;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED is active LOW on NodeMCU

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  server.on("/", HTTP_GET, serveHomePage);

  server.on("/login", HTTP_POST, []() {
    if (server.hasArg("credentials") && server.hasArg("iv")) {
      String encryptedMessage = server.arg("credentials");
      String iv = server.arg("iv");

      String credentials = decryptMessage(encryptedMessage, iv);
      Serial.println("Decoded Credentials: " + credentials);

      int colonIndex = credentials.indexOf(':');
      if (colonIndex == -1) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid credentials format\"}");
        return;
      }

      String username = credentials.substring(0, colonIndex);
      String password = credentials.substring(colonIndex + 1);

      if (username == "admin" && password == "password") {
        isLoggedIn = true;
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        isLoggedIn = false;
        server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid credentials\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing credentials or IV\"}");
    }
  });

  server.on("/led", HTTP_GET, []() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      return;
    }

    if (server.hasArg("state")) {
      String state = server.arg("state");
      if (state == "on") {
        digitalWrite(LED_PIN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (state == "off") {
        digitalWrite(LED_PIN, HIGH);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"success\":false}");
      }
    }
  });

  server.on("/logout", HTTP_GET, []() {
    isLoggedIn = false;
    server.send(200, "application/json", "{\"success\":true}");
  });

  server.begin();
  Serial.println("HTTP Server Started");
}

void loop() {
  server.handleClient();
}
