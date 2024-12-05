#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi configuration
#define WIFI_SSID "infinix7"
#define WIFI_PASSWORD "chater123"

// Backend configuration 
// We're using the npm package "Local Tunnel" to host the backend locally on our pc
// download it using : npm install -g localtunnel
// then Run the backend, then run: lt --port 5000 --subdomain ias-supcom
#define BACKEND_URL "http://ias-supcom.loca.lt/data"

// Define pins for the sensors
#define DHT1_PIN 33        // First DHT11 sensor
#define DHT2_PIN 32        // Second DHT11 sensor
#define MQ135_PIN 34       // MQ135 sensor
#define ACS712_1_PIN 35    // ACS712 sensor for industry 1
#define Industry1_Voltage 23
#define ACS712_2_PIN 36    // ACS712 sensor for industry 2
#define Industry2_Voltage 22
#define LED_PIN 18          // Built-in LED on most ESP32 boards

#define MQ135_THRESHOLD 520
#define DHT_TYPE DHT11

DHT dht1(DHT1_PIN, DHT_TYPE);
DHT dht2(DHT2_PIN, DHT_TYPE);

unsigned long sendDataPrevMillis = 0;
const long interval = 5000;

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(ACS712_1_PIN, INPUT);
  pinMode(ACS712_2_PIN, INPUT);
  
  dht1.begin();
  dht2.begin();
  Serial.println("Sensors initialized");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected with IP:");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
}

void sendDataToBackend(JsonDocument& doc) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }
    
    http.end();
  }
}

void loop() {
  int mq135_value = analogRead(MQ135_PIN);
  if (millis() - sendDataPrevMillis > interval || sendDataPrevMillis == 0) {
    sendDataPrevMillis = millis();
    
    StaticJsonDocument<1024> doc;
    JsonObject root = doc.createNestedObject("industrial_zones");
    JsonObject zone1 = root.createNestedObject("zone_1");
    
    float temp1 = dht1.readTemperature();
    float hum1 = dht1.readHumidity();
    // W = A * V;  V = read input * 2 (diviseur de courant) 
    int energy1 = analogRead(ACS712_1_PIN) * analogRead(Industry1_Voltage) * 2;
    
    float temp2 = dht2.readTemperature();
    float hum2 = dht2.readHumidity();
    int energy2 = analogRead(ACS712_2_PIN) * analogRead(Industry2_Voltage) * 2;
    
    mq135_value = analogRead(MQ135_PIN);
    
    if (mq135_value > MQ135_THRESHOLD) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }


    JsonObject industries = zone1.createNestedObject("industries");
    
    if (!isnan(temp1) && !isnan(hum1)) {
      JsonObject industry1 = industries.createNestedObject("industry_1");
      industry1["temperature"] = temp1;
      industry1["humidity"] = hum1;
      JsonObject energy_consumption = industry1.createNestedObject("energy_consumption");
      energy_consumption["batteries"] = energy1;
      energy_consumption["solar panels"] = energy1 * 0.47;
      energy_consumption["hydrogen"] = 0;
      
      Serial.println("Industry 1 readings:");
      Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Energy: %d\n", temp1, hum1, energy1);
    }
    
    if (!isnan(temp2) && !isnan(hum2)) {
      JsonObject industry2 = industries.createNestedObject("industry_2");
      industry2["temperature"] = temp2;
      industry2["humidity"] = hum2;
      industry2["energy_consumption"] = energy2;
      JsonObject energy_consumption2 = industry2.createNestedObject("energy_consumption");
      energy_consumption2["batteries"] = energy2;
      energy_consumption2["solar panels"] = (energy2 * 0.57).t;
      energy_consumption2["hydrogen"] = 0;
      
      Serial.println("Industry 2 readings:");
      Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Energy: %d\n", temp2, hum2, energy2);
    }
    
    zone1["gaz_emission"] = mq135_value;
    zone1["timestamp"] = millis();
    
    sendDataToBackend(doc);
  }
  Serial.println(mq135_value);
  delay(2000);
}