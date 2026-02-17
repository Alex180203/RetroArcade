#include <ESP8266WiFi.h>
#include <espnow.h> 
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

extern "C" {
  #include "user_interface.h"
}

const char* ssid = "Redmi Note 13 Pro+";          
const char* password = "1234567890";  
String serverUrl = "http://10.210.56.173:5000/move"; 

struct ButtonData { int dx; int dy; };
ButtonData data = {0, 0};

const int buttonPinUp    = 5;  // D1
const int buttonPinDown  = 4;  // D2
const int buttonPinLeft  = 14; // D5
const int buttonPinRight = 12; // D6
const int buttonPinMode  = 13; // D7

uint8_t receiverMac[] = {0x4C,0xC3,0x82,0x0C,0x7D,0x40};

bool useESPNow = true;

int lastStateUp = HIGH;
int lastStateDown = HIGH;
int lastStateLeft = HIGH;
int lastStateRight = HIGH;
int lastStateMode = HIGH; 

unsigned long lastDebounceUp = 0;
unsigned long lastDebounceDown = 0;
unsigned long lastDebounceLeft = 0;
unsigned long lastDebounceRight = 0;
unsigned long lastDebounceMode = 0;
unsigned long debounceDelay = 50;

void setup() {
    Serial.begin(115200);
    
    pinMode(buttonPinUp, INPUT_PULLUP);
    pinMode(buttonPinDown, INPUT_PULLUP);
    pinMode(buttonPinLeft, INPUT_PULLUP);
    pinMode(buttonPinRight, INPUT_PULLUP);
    pinMode(buttonPinMode, INPUT_PULLUP); 

   WiFi.mode(WIFI_STA); // Important: Radio must be on
    WiFi.disconnect();   // Ensure we aren't trying to connect to a Router
    
    if (esp_now_init() == 0) { 
        esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
        esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
        
        wifi_set_channel(1); 
        if (esp_now_init() == 1) {
        esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

        esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    }
}
}
void connectToWiFi() {
    Serial.print("\nConnecting to "); Serial.print(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int t = 0;
    while (WiFi.status() != WL_CONNECTED && t < 20) {
        delay(500); Serial.print("."); t++;
    }
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP: "); Serial.println(WiFi.localIP());
    }
}

void switchToEspNowMode() {
    sendHandshake("disconnected");
    WiFi.disconnect();
    delay(100);
    wifi_set_channel(1);
    
    data.dx = 99;
    data.dy = 99;
    esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));

    Serial.println("Mode: ESP-NOW (WiFi Disconnected, Channel 1)");
}

void switchToFlaskMode() {
    Serial.println("Mode: FLASK (Connecting to WiFi...)");
    connectToWiFi();
    sendHandshake("connected");
}

void sendHandshake(String status) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, serverUrl);
        http.addHeader("Content-Type", "application/json");
        String json = "{\"action\":\"" + status + "\"}";
        http.POST(json);
        http.end();
    }
}

void sendToFlask(int dx, int dy) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, serverUrl);
        http.addHeader("Content-Type", "application/json");
        String json = "{\"dx\":" + String(dx) + ", \"dy\":" + String(dy) + "}";
        http.POST(json);
        http.end();
    }
}

void sendMovement(int dx, int dy, const char* dir) {
    data.dx = dx; 
    data.dy = dy;
    if (useESPNow) {
        esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));
    } else {
        sendToFlask(dx, dy);
    }
}

void loop() {
    unsigned long currentMillis = millis();
    int readUp = digitalRead(buttonPinUp);
    if (readUp != lastStateUp && (currentMillis - lastDebounceUp) > debounceDelay) {
        lastDebounceUp = currentMillis;
        if (readUp == LOW) 
          sendMovement(0, -1, "Up");
        lastStateUp = readUp;
    }

    int readDown = digitalRead(buttonPinDown);
    if (readDown != lastStateDown && (currentMillis - lastDebounceDown) > debounceDelay) {
        lastDebounceDown = currentMillis;
        if (readDown == LOW) 
          sendMovement(0, 1, "Down");
        lastStateDown = readDown;
    }

    int readLeft = digitalRead(buttonPinLeft);
    if (readLeft != lastStateLeft && (currentMillis - lastDebounceLeft) > debounceDelay) {
        lastDebounceLeft = currentMillis;
        if (readLeft == LOW) 
          sendMovement(-1, 0, "Left");
        lastStateLeft = readLeft;
    }

    int readRight = digitalRead(buttonPinRight);
    if (readRight != lastStateRight && (currentMillis - lastDebounceRight) > debounceDelay) {
        lastDebounceRight = currentMillis;
        if (readRight == LOW) 
          sendMovement(1, 0, "Right");
        lastStateRight = readRight;
    }

    int readMode = digitalRead(buttonPinMode);
    if (readMode != lastStateMode && (currentMillis - lastDebounceMode) > debounceDelay) {
        lastDebounceMode = currentMillis;
        if (readMode == LOW) { 
            useESPNow = !useESPNow;
            
            if (useESPNow) {
                switchToEspNowMode();
            } else {
                switchToFlaskMode();
            }
        }
        lastStateMode = readMode;
    }
}