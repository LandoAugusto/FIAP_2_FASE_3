#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ===================== PINOS =====================
// Sensores Utilizados:
// DHT22: responsável pela leitura de temperatura (sensor principal do projeto).
// Botão: simula batimentos cardíacos (BPM).
// LED: indica visualmente cada batimento detectado.

#define BUTTON_PIN 22
#define LED_PIN 17
#define DHTPIN 15
#define DHTTYPE DHT22

// ===================== OBJETOS =====================
// Inicialização dos componentes principais do sistema:
// DHT: leitura de temperatura.
// WiFiClient: conexão com rede.
// PubSubClient: comunicação MQTT.

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// ===================== WIFI / MQTT =====================
// Configuração de rede e broker MQTT.
// MQTT atua como middleware baseado em broker, permitindo comunicação desacoplada.

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "192.168.0.120";
const int mqtt_port = 1883;

const char* mqtt_client_id = "esp32-fiap-01";

// Tópicos MQTT separados por tipo de dado (boa prática)
const char* mqtt_topic_temp = "sensor/temperatura";
const char* mqtt_topic_bpm  = "sensor/batimentos";

// ===================== BPM =====================
// Variáveis responsáveis pela lógica de batimentos cardíacos.

int beatCount = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

// Controle de debounce (evita múltiplas leituras falsas)
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Controle de tempo para cálculo de BPM
unsigned long lastBpmTime = 0;

// ===================== TEMPERATURA =====================
// Controle de tempo para leitura do sensor DHT22

unsigned long lastTempTime = 0;

// ===================== WIFI =====================
// Responsável por conectar o ESP32 à rede Wi-Fi.
// Implementa tentativa limitada de conexão (resiliência básica).

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
    Serial.println("\n❌ Falha no WiFi (modo offline)");
  }
}

// ===================== MQTT =====================
// Garante conexão contínua com o broker MQTT.
// Caso desconecte, tenta reconectar automaticamente.

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

// Publica mensagem MQTT apenas se estiver conectado
bool publishMQTT(const char* topic, const String& payload) {
  if (!client.connected()) return false;
  return client.publish(topic, payload.c_str());
}

// ===================== BPM =====================
// Processamento de batimentos cardíacos simulados.
// Utiliza botão como entrada e LED como feedback visual.
// Calcula BPM a cada 10 segundos.

void processBPM() {

  bool reading = digitalRead(BUTTON_PIN);

  // Detecta mudança no estado do botão
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Aplica debounce
  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {
      buttonState = reading;

      // Detecta batimento (pressionamento)
      if (buttonState == LOW) {
        beatCount++;

        // Feedback visual
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);

        Serial.print("💓 Batimento: ");
        Serial.println(beatCount);
      }
    }
  }

  lastButtonState = reading;

  // Cálculo de BPM a cada 10 segundos
  if (millis() - lastBpmTime >= 10000) {

    int bpm = beatCount * 6;

    Serial.print("❤️ BPM: ");
    Serial.println(bpm);

    // Estrutura JSON para envio MQTT
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
// Realiza leitura do sensor DHT22 a cada 10 segundos.
// Inclui validação para evitar dados inválidos.

void processTemperature() {

  if (millis() - lastTempTime < 10000) return;

  float temp = dht.readTemperature();

  // Validação da leitura
  if (isnan(temp)) {
    Serial.println("⚠️ Falha no DHT22");
    lastTempTime = millis();
    return;
  }

  Serial.print("🌡️ Temp: ");
  Serial.print(temp);
  Serial.println(" °C");

  // Estrutura JSON para envio MQTT
  StaticJsonDocument<128> doc;
  doc["temperatura"] = temp;

  String payload;
  serializeJson(doc, payload);

  publishMQTT(mqtt_topic_temp, payload);

  lastTempTime = millis();
}

// ===================== SETUP =====================
// Inicialização do sistema (Edge Computing).
// Configura sensores, rede e comunicação.

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
// Loop principal do sistema.
// Executa continuamente leitura e envio de dados.

void loop() {

  // Garante conexão MQTT (resiliência)
  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  // Processamento local (Edge)
  processBPM();
  processTemperature();
}

// ===================== FLUXO COMPLETO DO SISTEMA =====================
//
// O sistema segue uma arquitetura distribuída baseada em IoT,
// dividida em três camadas principais:
//
// --------------------------------------------------------------------
// 🧠 EDGE COMPUTING (ESP32)
// --------------------------------------------------------------------
// O ESP32 atua como dispositivo de borda, sendo responsável por:
// - Coletar dados de temperatura (sensor DHT22)
// - Simular batimentos cardíacos (botão)
// - Processar localmente os dados (cálculo de BPM)
// - Validar leituras (ex: evitar valores inválidos do sensor)
// - Enviar os dados via protocolo MQTT
//
// --------------------------------------------------------------------
// 🌐 FOG COMPUTING (Node-RED)
// --------------------------------------------------------------------
// O Node-RED atua como camada intermediária (fog), sendo responsável por:
// - Receber os dados MQTT do ESP32
// - Converter e tratar os dados (JSON → formato estruturado)
// - Aplicar regras de negócio:
//      • Temperatura > 38°C → alerta de febre
//      • BPM > 120 → alerta de taquicardia
// - Encaminhar os dados para armazenamento (InfluxDB)
// - Exibir dados em tempo real em dashboards (gráficos, gauges e alertas)
//
// --------------------------------------------------------------------
// ☁️ CLOUD COMPUTING (InfluxDB + Grafana)
// --------------------------------------------------------------------
// InfluxDB:
// - Armazena os dados como séries temporais
// - Mantém histórico de temperatura e batimentos
// - Permite consultas eficientes ao longo do tempo
//
// Grafana:
// - Consome os dados do InfluxDB
// - Exibe dashboards interativos:
//      • Gráfico de temperatura
//      • Gráfico de batimentos (BPM)
//      • Indicadores visuais (gauge)
//      • Alertas automáticos
//
// --------------------------------------------------------------------
// 🔄 RESUMO DO FLUXO DE DADOS
// --------------------------------------------------------------------
// ESP32 → MQTT → Node-RED → InfluxDB → Grafana
//
// Esse fluxo garante:
// - Processamento distribuído
// - Baixa latência (Edge)
// - Regras em tempo real (Fog)
// - Armazenamento e análise histórica (Cloud)
//
// --------------------------------------------------------------------
// 🚀 RESULTADO FINAL
// --------------------------------------------------------------------
// O sistema simula um dispositivo vestível de monitoramento cardíaco,
// aplicando conceitos reais de IoT em saúde digital, com foco em:
// - Eficiência
// - Escalabilidade
// - Resiliência
// - Monitoramento em tempo real
//