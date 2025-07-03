#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define DHTPIN 18   // GPIO 2
#define DHTTYPE DHT22 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "TP-Link_7FEE";
const char* password = "24990073";
const char* serverURL = "http://192.168.0.4:8000/log";

void connectWiFi() {
  WiFi.begin(ssid, password);
  delay(500);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA/SCL пины

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Display initialized");
  display.display();

  dht.begin();
  connectWiFi();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  Serial.print("Temperature: " + String(temp, 2));

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (isnan(temp) || isnan(hum)) {
    display.setCursor(0, 0);
    display.println("Reading error on DHT22");
    display.display();
  }
  else {
    display.setCursor(0, 0);
    display.println("Temperature is " + String(temp));
    display.setCursor(0, 8);
    display.println("Humidity is " + String(hum));
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"temperature\":" + String(temp, 2) + ",";
    json += "\"humidity\":" + String(hum, 2) + ",";
    json += "\"timestamp\":\"" + String(millis()) + "\"";
    json += "}";

    int code = http.POST(json);
    Serial.print("POST code: ");
    Serial.println(code);
    Serial.println(http.getString());

    http.end();
  }
  
  delay(5000);
}

