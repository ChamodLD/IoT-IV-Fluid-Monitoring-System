#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// ——— Access Point credentials ————————————————————
const char* AP_SSID = "ScaleAP";
const char* AP_PASS = "scale1234";

// ——— Web server setup ————————————————————————
WebServer server(80);

// ——— OLED display setup —————————————————————
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ——— HX711 setup ——————————————————————————————
const int LOADCELL_DOUT_PIN = 23;  // DT → GPIO23
const int LOADCELL_SCK_PIN  = 19;  // SCK → GPIO19
HX711 scale;

// ——— Tare button setup ——————————————————————
const int ZERO_BUTTON_PIN = 4;     // Pushbutton to GND
float weightOffset = 0.0f;

// ——— Serve web page with current weight ——————————
void handleRoot() {
  float raw   = scale.get_units(10);
  float adj   = raw - weightOffset;
  int   w     = (int)round(adj);

  String html =
    "<!DOCTYPE html><html>"
    "<head><meta http-equiv='refresh' content='1'/>"
    "<title>ESP32 Scale</title></head>"
    "<body style='font-family:Arial; text-align:center; margin:2em;'>"
    "<h1>Weight: " + String(w) + " g</h1>"
    "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  // ——— I2C begin for OLED on ESP32-WROOM default I2C pins
  Wire.begin(21, 22);  // SDA=21, SCL=22

  // ——— Initialize OLED ——————————————————————
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true) delay(100);
  }

  // ——— Show startup info ——————————————————————
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Hotspot SSID:");
  display.println(AP_SSID);
  display.println();
  display.println("Password:");
  display.println(AP_PASS);
  display.display();
  delay(6000);

  // ——— Start Wi-Fi AP —————————————————————————
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: "); Serial.println(apIP);

  // ——— Display connection hint ——————————————————
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connect to:");
  display.println(AP_SSID);
  display.println("Then go to:");
  display.setTextSize(2);
  display.setCursor(0, 32);
  display.println("http://" + apIP.toString());
  display.display();
  delay(6000);

  // ——— Init web server ————————————————————————
  server.on("/", handleRoot);
  server.begin();

  // ——— HX711 begin ————————————————————————————
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(412.55819f);  // Use your own calibration factor
  scale.tare();                 // Tare with empty hook
  weightOffset = 1170.0f;       // ← Added to zero initial 1170g reading

  // ——— Setup tare button ————————————————————————
  pinMode(ZERO_BUTTON_PIN, INPUT_PULLUP);  // active low
}

void loop() {
  server.handleClient();

  // ——— Check for tare button press ———————————————
  if (digitalRead(ZERO_BUTTON_PIN) == LOW) {
    delay(50);  // debounce
    if (digitalRead(ZERO_BUTTON_PIN) == LOW) {
      weightOffset = scale.get_units(10);
      Serial.print("New zero offset: ");
      Serial.println(weightOffset);

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(10, 20);
      display.println("Zeroed!");
      display.display();
      delay(1000);

      while (digitalRead(ZERO_BUTTON_PIN) == LOW) delay(10);  // wait for release
    }
  }

  // ——— Measure weight ——————————————————————————
  float raw = scale.get_units(10);
  float adj = raw - weightOffset;
  int w = (int)round(adj);

  // ——— Display weight on OLED ————————————————————
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Weight:");
  display.setTextSize(3);
  display.setCursor(0, 28);
  display.print(w);
  display.print("g");
  display.display();

  delay(200);
}
