#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== WIFI =====
const char* ssid = "Shreya";
const char* password = "12345678";

// 🔥 Paste your Google Script URL here
String googleScriptURL = "https://script.google.com/macros/s/AKfycbyhwmfQCYCZx7sUFxYEvoCmluXL-bli4xl7nkmKkw4O9G-vMypW9J6PiQM9mwL0YJki1A/exec";

// ===== ECG PINS =====
int ecgPin = 4;
int loPlus = 5;
int loMinus = 18;

// ===== VARIABLES =====
unsigned long lastUpload = 0;
unsigned long lastBeat = 0;

int beatCount = 0;
int bpm = 0;

int threshold = 2000;

// OLED graph
int x = 0;
int prevY = 32;

// ===== FUNCTION TO SEND DATA =====
void sendToGoogleSheets(int bpm, int ecg)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    String url = googleScriptURL + "?bpm=" + String(bpm) + "&ecg=" + String(ecg);

    http.begin(url);
    http.GET();
    http.end();
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(loPlus, INPUT);
  pinMode(loMinus, INPUT);

  // ===== WIFI CONNECT =====
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected");

  // ===== OLED INIT =====
  Wire.begin(8, 9);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED failed");
    while (1);
  }

  display.clearDisplay();
  display.display();
}

void loop()
{
  // ===== LEAD CHECK =====
  if (digitalRead(loPlus) == 1 || digitalRead(loMinus) == 1)
  {
    Serial.println("Leads Off!");
    delay(200);
    return;
  }

  int ecgValue = analogRead(ecgPin);

  // ===== SERIAL PLOTTER =====
  Serial.println(ecgValue);

  // ===== BPM DETECTION (UNCHANGED) =====
  if (ecgValue > threshold && millis() - lastBeat > 300)
  {
    beatCount++;
    lastBeat = millis();
  }

  // ===== OLED GRAPH =====
  int y = map(ecgValue, 1800, 2600, 63, 16);
  y = constrain(y, 16, 63);

  display.drawLine(x - 1, prevY, x, y, WHITE);

  prevY = y;
  x++;

  if (x >= 128)
  {
    x = 0;
    display.fillRect(0, 16, 128, 48, BLACK);
  }

  // ===== BPM EVERY 10 SECONDS =====
  if (millis() - lastUpload > 10000)
  {
    bpm = beatCount * 6;

    Serial.print("BPM: ");
    Serial.println(bpm);

    // ===== SEND TO GOOGLE SHEETS =====
    sendToGoogleSheets(bpm, ecgValue);

    // ===== OLED TEXT =====
    display.fillRect(0, 0, 128, 16, BLACK);
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.print("BPM: ");
    display.print(bpm);

    beatCount = 0;
    lastUpload = millis();
  }

  display.display();

  delay(50);
}