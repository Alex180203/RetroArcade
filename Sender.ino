#include <ESP8266WiFi.h>
#include <espnow.h>

struct ButtonData { int dx; int dy; };
ButtonData data = {0, 0};

// Define button pins
const int buttonPinUp = 2;    
const int buttonPinDown = 6;  
const int buttonPinLeft = 7;  
const int buttonPinRight = 8;  

uint8_t receiverMac[] = { 0xCC, 0xC3, 0x82, 0x0C, 0x7D, 0x40 };

bool prevButtonStateUp = HIGH;
bool prevButtonStateDown = HIGH;
bool prevButtonStateLeft = HIGH;
bool prevButtonStateRight = HIGH;

unsigned long lastDebounceTimeUp = 0;
unsigned long lastDebounceTimeDown = 0;
unsigned long lastDebounceTimeLeft = 0;
unsigned long lastDebounceTimeRight = 0;
unsigned long debounceDelay = 50;  

void setup() {
    Serial.begin(115200);
    
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    pinMode(buttonPinUp, INPUT_PULLUP);
    pinMode(buttonPinDown, INPUT_PULLUP);
    pinMode(buttonPinLeft, INPUT_PULLUP);
    pinMode(buttonPinRight, INPUT_PULLUP);

    Serial.println("Sender is ready.");
}

void loop() {
    unsigned long currentMillis = millis();
    
    if (digitalRead(buttonPinUp) == LOW && (currentMillis - lastDebounceTimeUp) > debounceDelay) {
        if (prevButtonStateUp == HIGH) 
        { 
            data.dx = 0; data.dy = 1;  
            sendMovement("Up");
            prevButtonStateUp = LOW;  
        }
        lastDebounceTimeUp = currentMillis;
    } 
    else if (digitalRead(buttonPinDown) == LOW && (currentMillis - lastDebounceTimeDown) > debounceDelay) {
        if (prevButtonStateDown == HIGH) 
        {
            data.dx = 0; data.dy = -1;  
            sendMovement("Down");
            prevButtonStateDown = LOW;
        }
        lastDebounceTimeDown = currentMillis;
    } 
    else if (digitalRead(buttonPinLeft) == LOW && (currentMillis - lastDebounceTimeLeft) > debounceDelay) {
        if (prevButtonStateLeft == HIGH) 
        {
            data.dx = -1; data.dy = 0;  
            sendMovement("Left");
            prevButtonStateLeft = LOW;
        }
        lastDebounceTimeLeft = currentMillis;
    } 
    else if (digitalRead(buttonPinRight) == LOW && (currentMillis - lastDebounceTimeRight) > debounceDelay) {
        if (prevButtonStateRight == HIGH) 
        {
            data.dx = 1; data.dy = 0;  
            sendMovement("Right");
            prevButtonStateRight = LOW;
        }
        lastDebounceTimeRight = currentMillis;
    }
    else {
        if (digitalRead(buttonPinUp) == HIGH) 
        {
          prevButtonStateUp = HIGH;
        }
        if (digitalRead(buttonPinDown) == HIGH) 
        {
          prevButtonStateDown = HIGH;
        }
        if (digitalRead(buttonPinLeft) == HIGH) 
        {
          prevButtonStateLeft = HIGH;
        }
        if (digitalRead(buttonPinRight) == HIGH) 
        {
          prevButtonStateRight = HIGH;
        }
    }
}

void sendMovement(const char* direction) {
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));
    
    if (result == ESP_OK) {
        Serial.print("Movement sent: ");
        Serial.println(direction);
    } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
        Serial.println("ESP-NOW not initialized!");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
        Serial.println("Invalid argument to send!");
    } else {
        Serial.println("Movement failed, retrying...");
    }
}
