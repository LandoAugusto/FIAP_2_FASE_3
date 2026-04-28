#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

// ===== CONFIG =====
#define DHTPIN 4
#define DHTTYPE DHT22
#define BUTTON_PIN 5
#define LED_PIN 2

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

// ===== MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);

// ===== SENSOR =====
DHT dht(DHTPIN, DHTTYPE);

// ===== VARIÁVEIS =====
float temperatura = 0;
float umidade = 0;
int bpm = 0;

int beatCount = 0;
unsigned long lastBeatTime = 0;
unsigned long lastCalcTime = 0;

// ===== CONTROLE TEMPO =====
unsigned long lastPublish = 0;
unsigned long lastDHTRead = 0;

// ===== FILA OFFLINE =====
#define MAX_QUEUE 10

struct Mensagem {
  String topico;
  String payload;
};

Mensagem fila[MAX_QUEUE];
int filaIndex = 0;

// ===== WIFI =====
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

// ===== MQTT =====
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Erro: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}



// ===== FILA =====
void adicionarFila(const char* topico, String payload) {
  if (filaIndex < MAX_QUEUE) {
    fila[filaIndex].topico = topico;
    fila[filaIndex].payload = payload;
    filaIndex++;
  } else {
    Serial.println("⚠️ Fila cheia, descartando dados");
  }
}

void enviarOuArmazenar(const char* topico, String payload) {
  if (client.connected()) {
    bool ok = client.publish(topico, payload.c_str());
    if (!ok) {
      Serial.println("❌ Falha, salvando na fila...");
      adicionarFila(topico, payload);
    }
  } else {
    Serial.println("⚠️ Offline, salvando na fila...");
    adicionarFila(topico, payload);
  }
}

void processarFila() {
  if (!client.connected()) return;

  for (int i = 0; i < filaIndex; i++) {
    client.publish(fila[i].topico.c_str(), fila[i].payload.c_str());
  }
  if (filaIndex > 0) {
    Serial.println("📤 Fila reenviada com sucesso");
  }

  filaIndex = 0;
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Serial.println("Sistema iniciado");
}

// ===== LOOP =====
void loop() {
  // 🔌 MQTT
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  unsigned long now = millis();
  // ❤️ BATIMENTOS (botão)
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (now - lastBeatTime > 300) {
      beatCount++;
      lastBeatTime = now;
    }
  } 
  // ⏱️ Calcula BPM a cada 10s
  if (now - lastCalcTime >= 10000) {
    bpm = beatCount * 6;
    beatCount = 0;
    lastCalcTime = now;
  }

  // 🌡️ Leitura DHT (2s)
  if (now - lastDHTRead >= 2000) {
    lastDHTRead = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      umidade = h;
      temperatura = t;
    } else {
      Serial.println("❌ Erro leitura DHT");
    }
  }

  // 🚨 Lógica local (Edge)
  if (temperatura > 38 || bpm > 120) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // 📡 Publicação (5s)
  if (now - lastPublish >= 5000) {
    lastPublish = now;

    // Temperatura
    enviarOuArmazenar("esp32/saude/temperatura", String(temperatura));
    // BPM
    enviarOuArmazenar("esp32/saude/bpm", String(bpm));
    // Alerta
    if (temperatura > 38 || bpm > 120) {
      enviarOuArmazenar("esp32/saude/alerta", "CRITICO");
    } else {
      enviarOuArmazenar("esp32/saude/alerta", "NORMAL");
    }

    // SERIAL (fallback offline)
    Serial.print("Temp: ");
    Serial.print(temperatura);
    Serial.print(" | BPM: ");
    Serial.println(bpm);
  }

  // 🔁 Retry fila
  processarFila();
}