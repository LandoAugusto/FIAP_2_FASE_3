#include <DHT.h>
#include <ArduinoJson.h>

// ===================== PINOS =====================
#define DHTPIN 15
#define DHTTYPE DHT22
#define PIR_PIN 4

DHT dht(DHTPIN, DHTTYPE);

// ===================== CONTROLE =====================
unsigned long lastRead = 0;
const long interval = 5000;

// 🔥 SIMULAÇÃO DE CONECTIVIDADE
bool isOnline = true;

// 🔥 BUFFER LOCAL (RESILIÊNCIA)
#define MAX_BUFFER 20
String buffer[MAX_BUFFER];
int bufferIndex = 0;

// ===================== SALVAR LOCAL =====================
void saveLocal(String payload) {
  if (bufferIndex < MAX_BUFFER) {
    buffer[bufferIndex++] = payload;
    Serial.println("💾 Dados salvos localmente");
  } else {
    Serial.println("⚠️ Buffer cheio - aplicando FIFO");

    for (int i = 1; i < MAX_BUFFER; i++) {
      buffer[i - 1] = buffer[i];
    }
    buffer[MAX_BUFFER - 1] = payload;
  }
}

// ===================== SINCRONIZAR =====================
void syncData() {
  if (bufferIndex == 0) return;

  Serial.println("🔄 Sincronizando dados armazenados...");

  for (int i = 0; i < bufferIndex; i++) {
    Serial.print("📤 Enviado: ");
    Serial.println(buffer[i]);
    delay(200);
  }

  bufferIndex = 0;
  Serial.println("✅ Buffer limpo");
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(PIR_PIN, INPUT);

  delay(2000); // estabilização do DHT

  Serial.println("🚀 Sistema Edge iniciado (Wokwi)");
}

// ===================== LOOP =====================
void loop() {

  // 🔥 alterna ONLINE/OFFLINE a cada 15s (simulação)
  if (millis() % 30000 < 15000) {
    isOnline = true;
  } else {
    isOnline = false;
  }

  if (millis() - lastRead < interval) return;
  lastRead = millis();

  // ===================== LEITURA DHT =====================
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("❌ Erro leitura DHT");
    return;
  }

  // ===================== PIR =====================
  int motion = digitalRead(PIR_PIN);

  // ===================== JSON =====================
  StaticJsonDocument<200> doc;
  doc["temperatura"] = temp;
  doc["umidade"] = hum;
  doc["movimento"] = (motion == HIGH);

  String payload;
  serializeJson(doc, payload);

  Serial.println("------------");
  Serial.println(payload);

  // ===================== REGRAS DE NEGÓCIO =====================
  if (temp > 38) {
    Serial.println("⚠️ ALERTA: FEBRE DETECTADA");
  }

  if (motion == HIGH) {
    Serial.println("⚠️ ALERTA: PRESENÇA DETECTADA");
  }

  // ===================== EDGE LOGIC =====================
  if (isOnline) {

    Serial.println("🌐 ONLINE → enviando dados (Serial)");

    Serial.print("📤 ");
    Serial.println(payload);

    // 🔄 sincroniza dados antigos
    syncData();

  } else {

    Serial.println("📴 OFFLINE → armazenando local");
    saveLocal(payload);
  }
}