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

// --- KONFIGURASI PIN ---
#define DHTPIN D7 // GPIO2
#define DHTTYPE DHT11
#define FAN_PIN D5    // GPIO14
#define BUZZER_PIN D6 // GPIO12

// --- KONFIGURASI OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- OBJEK SENSOR & SERVER ---
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);
WebSocketsServer webSocket(81);

// --- VARIABEL GLOBAL ---
float currentTemp = 0.0;
float currentHum = 0.0;
int currentPWM = 0;
String currentMode = "AUTO";

// Variabel Waktu (Non-blocking menggunakan millis())
unsigned long lastSensorRead = 0;
unsigned long lastWebSocketSend = 0;
unsigned long lastAnimUpdate = 0;
unsigned long lastBuzzerToggle = 0;
unsigned long lastChartUpdate = 0;

// Status Buzzer Alarm
bool buzzerState = false;

// Array Animasi Angin (Dummy PROGMEM 32x32 pixel, 4 frame)
// Array diisi dengan nol, dapat diganti dengan byte hasil generate Wokwi
// Animator
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

// Data Sparkline (60 data points untuk 1 menit terakhir)
float tempHistory[60];

// --- KONFIGURASI WIFI ---
const char *ssid = "X6_KEL5_SMARTFAN";
const char *password = "12345678";

// --- WEBSOCKET PARSER ---
void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length) {
  // Parse JSON dari Klien
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // Cek perintah penggantian mode
  if (doc.containsKey("mode")) {
    String newMode = doc["mode"].as<String>();
    if (newMode == "AUTO" || newMode == "MANUAL") {
      currentMode = newMode;
    }
  }

  // Cek perintah penggantian PWM saat mode MANUAL
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

  // Inisialisasi Pin Aktuator
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  analogWriteRange(255); // Set range ke 0-255 agar sinkron dengan variabel currentPWM
  analogWrite(FAN_PIN, 0);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Startup Check: Professional Chime
  // Arpeggio: C6 -> E6 -> G6 -> C7
  int melody[] = {1047, 1319, 1568, 2093};
  int durations[] = {80, 80, 80, 200};
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i]);
    delay(durations[i]);
  }
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH); // Ensure OFF (Active Low)
  Serial.println(F("Buzzer check... OK (Professional Chime)"));

  // Inisialisasi Sensor Suhu & Kelembaban
  dht.begin();

  // Inisialisasi OLED (I2C: SDA=D2, SCL=D1)
  Wire.begin(D2, D1);
  Wire.setClock(400000); // Tingkatkan kecepatan I2C ke 400kHz untuk animasi lebih smooth
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Berhenti jika OLED gagal inisialisasi
  }
  display.clearDisplay();
  display.display();

  // Inisialisasi History dengan nilai 0
  for (int i = 0; i < 60; i++)
    tempHistory[i] = 0;

  // Inisialisasi LittleFS untuk web server
  if (!LittleFS.begin()) {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }

  // WiFi Setup (Access Point Mode)
  WiFi.softAP(ssid, password);
  Serial.print(F("AP Started. IP: "));
  Serial.println(WiFi.softAPIP());

  // Definisi Rute Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Melayani file statis lainnya (css, js, library) secara otomatis
  server.serveStatic("/", LittleFS, "/");

  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  webSocket.enableHeartbeat(15000, 3000, 2); 
}

void loop() {
  webSocket.loop();
  unsigned long currentMillis = millis();

  // 1. Baca Sensor (interval 2 detik)
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

  // 2. Logika Kontrol Kipas & Buzzer
  if (currentMode == "AUTO") {
    // Logika perhitungan PWM berdasarkan Suhu
    if (currentTemp < 28.0) {
      currentPWM = 0;
    } else if (currentTemp > 35.0) {
      currentPWM = 255;
    } else {
      // Pemetaan proporsional antara 28C - 35C ke PWM 0 - 255
      currentPWM = map(currentTemp * 10, 280, 350, 0, 255);
    }
  }

  // Terapkan nilai PWM ke pin Mosfet Kipas
  analogWrite(FAN_PIN, currentPWM);

  // 3. Logika Alarm Buzzer (Non-blocking Sophisticated Pattern)
  static unsigned long lastAlarmStep = 0;
  static int alarmStep = 0;

  if (currentTemp > 33.0) {
    unsigned long alarmInterval;
    // Tentukan interval tiap langkah (Double-chirp then long pause)
    switch (alarmStep) {
      case 0: alarmInterval = 80; break;  // Chirp 1 duration
      case 1: alarmInterval = 50; break;  // Gap duration
      case 2: alarmInterval = 80; break;  // Chirp 2 duration
      default: alarmInterval = 1500; break; // Long pause
    }

    if (currentMillis - lastAlarmStep >= alarmInterval) {
      lastAlarmStep = currentMillis;
      alarmStep = (alarmStep + 1) % 4;
      
      if (alarmStep == 1 || alarmStep == 3) {
        tone(BUZZER_PIN, 2800); // Freq tinggi agar terdengar profesional
      } else {
        noTone(BUZZER_PIN);
        digitalWrite(BUZZER_PIN, HIGH); // OFF state
      }
    }
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, HIGH);
    alarmStep = 0;
  }

  // 3. Kirim WebSocket Data (interval 1 detik)
  if (currentMillis - lastWebSocketSend >= 1000) {
    lastWebSocketSend = currentMillis;

    // Update array Sparkline Chart (geser data ke kiri)
    for (int i = 0; i < 59; i++) {
      tempHistory[i] = tempHistory[i + 1];
    }
    tempHistory[59] = currentTemp;

    // Buat objek JSON dan kirim
    StaticJsonDocument<200> doc;
    doc["temp"] = currentTemp;
    doc["hum"] = currentHum;
    doc["pwm"] = currentPWM;
    doc["mode"] = currentMode;

    char buffer[200];
    serializeJson(doc, buffer);
    webSocket.broadcastTXT(buffer);
  }

  // 4. Update OLED Display & Animasi
  int animDelay = 0;
  if (currentPWM > 0) {
    // Mapping kecepatan frame animasi terhadap PWM 
    // Range disesuaikan: PWM 0 -> 500ms (Lambat), PWM 255 -> 100ms (Cepat)
    animDelay = map(currentPWM, 0, 255, 500, 100);
  }

  // Update frame animasi tanpa blocking code
  if (currentPWM > 0 && (currentMillis - lastAnimUpdate >= animDelay)) {
    lastAnimUpdate = currentMillis;
    currentFrame = (currentFrame + 1) % NUM_FRAMES;
  }

  // Render ulang layar OLED setiap 200ms agar efisien dan tidak over-flicker
  static unsigned long lastDisplayUpdate = 0;
  if (currentMillis - lastDisplayUpdate >= 200) {
    lastDisplayUpdate = currentMillis;
    display.clearDisplay();

    // Gambar Animasi Angin di (0,0) dengan ukuran 32x32
    display.drawBitmap(0, 0, wind_frames[currentFrame], 32, 32, SSD1306_WHITE);

    // Teks Suhu (Samping animasi, Size 2)
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(36, 0);
    display.print(currentTemp, 1);
    display.print("C");

    // Teks Kelembaban & PWM Persentase (Bawah suhu, Size 1)
    display.setTextSize(1);
    display.setCursor(36, 18);
    display.print("H:");
    display.print(currentHum, 1);
    display.print("% P:");
    display.print(map(currentPWM, 0, 255, 0, 100)); // Konversi ke %
    display.print("%");

    // Sparkline Chart (Area Y: 40 sampai 64)
    // Tinggi chart 24 pixel. Lebar X menggunakan jarak 2 pixel tiap titik (max
    // 60 titik = 120 width).
    int chartHeight = 24;
    int chartY = 64; // Batas bawah

    // Auto scale min max chart
    float minT = 100, maxT = -100;
    for (int i = 0; i < 60; i++) {
      if (tempHistory[i] > 0) {
        if (tempHistory[i] < minT)
          minT = tempHistory[i];
        if (tempHistory[i] > maxT)
          maxT = tempHistory[i];
      }
    }
    // Padding min/max agar grafis stabil
    if (maxT - minT < 5) {
      maxT += 2.5;
      minT -= 2.5;
    }

    // Gambar garis antartitik (Sparkline)
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
