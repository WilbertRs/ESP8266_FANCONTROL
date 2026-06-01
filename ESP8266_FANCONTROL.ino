#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
#include <Wire.h>

#define DHTPIN D7
#define DHTTYPE DHT11
#define FAN_PIN D5
#define BUZZER_PIN D6

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);
WebSocketsServer webSocket(81);

float currentTemp = 0.0;
float currentHum = 0.0;
int currentPWM = 0;
String currentMode = "AUTO";

unsigned long lastSensorRead = 0;
unsigned long lastWebSocketSend = 0;
unsigned long lastAnimUpdate = 0;
unsigned long lastBuzzerToggle = 0;
unsigned long lastChartUpdate = 0;

bool buzzerState = false;

const unsigned char wind_frames[][128] PROGMEM = {
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,1,192,0,6,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,1,192,0,6,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,1,192,0,6,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,1,192,0,6,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,8,0,4,31,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,12,0,0,2,4,1,192,0,4,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,8,0,4,7,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,12,0,0,2,4,1,192,0,4,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,8,0,4,0,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,31,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,12,0,0,2,4,1,192,0,4,6,112,0,6,12,24,0,6,24,8,0,4,0,4,0,24,0,4,0,48,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,3,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,1,192,0,6,6,112,0,6,12,24,0,4,24,8,0,4,0,4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,127,192,0,0,0,48,0,0,0,16,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,12,0,0,2,4,1,192,0,4,6,112,0,2,12,24,0,0,24,8,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,8,0,0,0,24,3,255,255,224,3,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,12,0,0,2,0,1,192,0,0,6,112,0,0,12,24,0,0,24,8,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,8,0,0,0,24,0,15,255,224,0,15,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,0,0,0,2,0,1,192,0,0,7,112,0,0,12,24,0,0,24,8,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,8,0,0,0,24,0,0,63,224,0,0,63,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,3,0,0,0,2,0,1,192,0,0,7,112,0,0,12,24,0,0,24,8,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,8,0,0,0,24,0,0,0,224,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,2,0,1,192,0,0,7,112,0,0,12,24,0,0,24,8,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,3,128,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,0,0,6,112,0,0,12,0,0,0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,8,0,0,0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,128,0,0,63,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,248,0,0,63,248,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,240,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,63,255,192,0,0,0,0,0,0,0,0,0,0,0,0,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,4,0,0,0,8,0,0,63,240,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,254,0,63,255,254,0,0,0,0,0,0,0,0,0,0,0,0,0,63,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,4,0,0,0,6,0,0,0,4,0,0,0,4,0,0,0,8,0,0,63,240,0,0,0,0,0,0,0,0,0,0,0,0,0,16,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,12,0,0,0,4,0,0,0,6,0,0,0,6,0,0,0,4,0,0,0,4,0,0,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,0,8,0,0,0,4,0,0,0,6,0,0,0,6,0,24,0,4,0,12,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,0,192,0,6,0,112,0,6,0,24,0,4,0,8,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,0,48,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,0,192,0,6,6,112,0,6,0,24,0,4,0,8,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,0,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,240,0,0,3,8,0,0,2,4,1,192,0,6,6,112,0,6,8,24,0,4,0,8,0,4,0,4,0,8,0,4,63,240,0,4,0,0,0,4,0,0,0,8,0,0,0,24,31,255,255,224,31,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,63,255,192,0,0,0,48,0,0,0,24,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,24,0,0,4,48,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

int currentFrame = 0;
const int NUM_FRAMES = sizeof(wind_frames) / 128;

float tempHistory[60];

const char *ssid = "X6_KEL5_SMARTFAN";
const char *password = "12345678";

void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("mode")) {
    String newMode = doc["mode"].as<String>();
    if (newMode == "AUTO" || newMode == "MANUAL") {
      currentMode = newMode;
    }
  }

  if (currentMode == "MANUAL" && doc.containsKey("pwm")) {
    int newPWM = doc["pwm"].as<int>();
    if (newPWM >= 0 && newPWM <= 255) {
      currentPWM = newPWM;
    }
  }
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                      size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
    Serial.printf("[%u] Connected!\n", num);
    break;
  case WStype_TEXT:
    handleWebSocketMessage(num, payload, length);
    break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  analogWriteRange(255);
  analogWrite(FAN_PIN, 0);
  digitalWrite(BUZZER_PIN, LOW);
  
  int melody[] = {1047, 1319, 1568, 2093};
  int durations[] = {80, 80, 80, 200};
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i]);
    delay(durations[i]);
  }
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println(F("Buzzer check... OK (Professional Chime)"));

  dht.begin();

  Wire.begin(D2, D1);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.display();

  for (int i = 0; i < 60; i++)
    tempHistory[i] = 0;

  if (!LittleFS.begin()) {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }

  WiFi.softAP(ssid, password);
  Serial.print(F("AP Started. IP: "));
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  webSocket.enableHeartbeat(15000, 3000, 2); 
}

void loop() {
  webSocket.loop();
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorRead >= 2000) {
    lastSensorRead = currentMillis;
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println(F("Failed to read from DHT sensor! Check wiring."));
    } else {
      currentTemp = t;
      currentHum = h;
      Serial.printf("Sensor Read: T=%.1f C, H=%.1f %%\n", currentTemp, currentHum);
    }
  }

  if (currentMode == "AUTO") {
    if (currentTemp < 28.0) {
      currentPWM = 0;
    } else if (currentTemp > 35.0) {
      currentPWM = 255;
    } else {
      currentPWM = map(currentTemp * 10, 280, 350, 0, 255);
    }
  }

  analogWrite(FAN_PIN, currentPWM);

  static unsigned long lastAlarmStep = 0;
  static int alarmStep = 0;

  if (currentTemp > 33.0) {
    unsigned long alarmInterval;
    switch (alarmStep) {
      case 0: alarmInterval = 80; break;
      case 1: alarmInterval = 50; break;
      case 2: alarmInterval = 80; break;
      default: alarmInterval = 1500; break;
    }

    if (currentMillis - lastAlarmStep >= alarmInterval) {
      lastAlarmStep = currentMillis;
      alarmStep = (alarmStep + 1) % 4;
      
      if (alarmStep == 1 || alarmStep == 3) {
        tone(BUZZER_PIN, 2800);
      } else {
        noTone(BUZZER_PIN);
        digitalWrite(BUZZER_PIN, HIGH);
      }
    }
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, HIGH);
    alarmStep = 0;
  }

  if (currentMillis - lastWebSocketSend >= 1000) {
    lastWebSocketSend = currentMillis;

    for (int i = 0; i < 59; i++) {
      tempHistory[i] = tempHistory[i + 1];
    }
    tempHistory[59] = currentTemp;

    StaticJsonDocument<200> doc;
    doc["temp"] = currentTemp;
    doc["hum"] = currentHum;
    doc["pwm"] = currentPWM;
    doc["mode"] = currentMode;

    char buffer[200];
    serializeJson(doc, buffer);
    webSocket.broadcastTXT(buffer);
  }

  int animDelay = 0;
  if (currentPWM > 0) {
    animDelay = map(currentPWM, 0, 255, 500, 100);
  }

  if (currentPWM > 0 && (currentMillis - lastAnimUpdate >= animDelay)) {
    lastAnimUpdate = currentMillis;
    currentFrame = (currentFrame + 1) % NUM_FRAMES;
  }

  static unsigned long lastDisplayUpdate = 0;
  if (currentMillis - lastDisplayUpdate >= 200) {
    lastDisplayUpdate = currentMillis;
    display.clearDisplay();

    display.drawBitmap(0, 0, wind_frames[currentFrame], 32, 32, SSD1306_WHITE);

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(36, 0);
    display.print(currentTemp, 1);
    display.print("C");

    display.setTextSize(1);
    display.setCursor(36, 18);
    display.print("H:");
    display.print(currentHum, 1);
    display.print("% P:");
    display.print(map(currentPWM, 0, 255, 0, 100));
    display.print("%");

    int chartHeight = 24;
    int chartY = 64;

    float minT = 100, maxT = -100;
    for (int i = 0; i < 60; i++) {
      if (tempHistory[i] > 0) {
        if (tempHistory[i] < minT)
          minT = tempHistory[i];
        if (tempHistory[i] > maxT)
          maxT = tempHistory[i];
      }
    }
    if (maxT - minT < 5) {
      maxT += 2.5;
      minT -= 2.5;
    }

    for (int i = 0; i < 59; i++) {
      if (tempHistory[i] > 0 && tempHistory[i + 1] > 0) {
        int x1 = i * 2;
        int y1 = chartY -
                 map(tempHistory[i] * 10, minT * 10, maxT * 10, 0, chartHeight);
        int x2 = (i + 1) * 2;
        int y2 = chartY - map(tempHistory[i + 1] * 10, minT * 10, maxT * 10, 0,
                              chartHeight);
        display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
      }
    }

    display.display();
  }
}
