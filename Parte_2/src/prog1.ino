#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ===================== PINOS =====================
#define BUTTON_PIN 22
#define LED_PIN 17
#define DHTPIN 15
#define DHTTYPE DHT22

// ===================== OBJETOS =====================
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// ===================== WIFI / MQTT =====================
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "192.168.0.120";
const int mqtt_port = 1883;
const char* mqtt_client_id = "esp32-fiap-01";
const char* mqtt_topic_temp = "sensor/temperatura";
const char* mqtt_topic_bpm  = "sensor/batimentos";

// ===================== BPM =====================
int beatCount = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastBpmTime = 0;

// ===================== TEMPERATURA =====================
unsigned long lastTempTime = 0;

// ===================== WIFI =====================
void setup_wifi() {
  Serial.print("Conectando em: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Falha no WiFi");
  }
}

// ===================== MQTT =====================
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");

    if (client.connect(mqtt_client_id)) {
      Serial.println("OK!");
    } else {
      Serial.print("Falha rc=");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

bool publishMQTT(const char* topic, const String& payload) {
  if (!client.connected()) return false;
  return client.publish(topic, payload.c_str());
}

// ===================== BPM =====================
void processBPM() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        beatCount++;

        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);

        Serial.print("💓 Batimento: ");
        Serial.println(beatCount);
      }
    }
  }

  lastButtonState = reading;

  // BPM a cada 10s
  if (millis() - lastBpmTime >= 10000) {
    int bpm = beatCount * 6;

    Serial.print("❤️ BPM: ");
    Serial.println(bpm);

    StaticJsonDocument<128> doc1;
    doc1["bpm"] = bpm;    
    String payload;
    serializeJson(doc1, payload);
    publishMQTT(mqtt_topic_bpm, payload);

    beatCount = 0;
    lastBpmTime = millis();
  }
}

// ===================== TEMPERATURA =====================
void processTemperature() {
  if (millis() - lastTempTime < 10000) return;

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ Falha no DHT22");
    lastTempTime = millis();
    return;
  }

  Serial.print("🌡️ Temp: ");
  Serial.print(temp);
  Serial.print(" °C | 💧 Umidade: ");
  Serial.print(hum);
  Serial.println(" %");

  StaticJsonDocument<128> doc;
  doc["temperatura"] = temp;
  doc["umidade"] = hum;

  String payload;
  serializeJson(doc, payload);

  publishMQTT(mqtt_topic_temp, payload);

  lastTempTime = millis();
}


// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("💓 Sistema iniciado (BPM + DHT22 + MQTT)");
}

// ===================== LOOP =====================
void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  processBPM();
  processTemperature();
}