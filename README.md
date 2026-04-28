# 🌾 Projeto Cap 1 - CardioIA Conectada: IoT e Visualização de Dados para a Saúde Digital

---

## 👨‍🎓 Integrantes

| Nome Completo                   | RM       |
| ------------------------------- | -------- |
| Daniele Antonieta Garisto Dias  | RM565106 |
| Leandro Augusto Jardim da Cunha | RM561395 |
| Luiz Eduardo da Silva           | RM561701 |
| João Victor Viana de Sousa      | RM565136 |

---

## 👩‍🏫 Professores

### Tutor(a)

- Leonardo Ruiz Orabona

### Coordenador(a)

- Andre Godoi Chiovato

---

# 🎯 Parte 2 — Transmissão para Nuvem e Visualização

A solução proposta consiste em um sistema de monitoramento em tempo real de sinais vitais, utilizando sensores conectados a um microcontrolador ESP32.
O sistema é responsável pela coleta das seguintes variáveis:

- ❤️ Batimentos cardíacos (BPM)
- 🌡️ Temperatura corporal
- 💧 Umidade

Os dados são processados localmente (camada Edge) e enviados através de uma camada de mensageria utilizando o protocolo MQTT, garantindo comunicação eficiente, leve e escalável.
Após a transmissão, os dados são encaminhados para a nuvem (Microsoft Azure), onde podem ser armazenados, analisados e visualizados por meio de dashboards.
Os dados são transmitidos via **MQTT**, permitindo integração com sistemas de visualização e nuvem.

---

# 🏗️ Arquitetura da Solução

```text
[ESP32] → [MQTT Broker] → [Node-RED] → [Azure]
```

### Camadas:

- **Edge**: ESP32 + sensores (coleta)
- **Fog**: Broker MQTT (HiveMQ)
- **Application**: Node-RED (processamento)
- **Cloud**: Azure (armazenamento e analytics)

---

# 🔄 Fluxo de Dados

1. ESP32 coleta dados dos sensores
2. Estrutura em JSON
3. Publica via MQTT
4. Node-RED consome
5. Azure armazena e analisa

---

# 📡 Estrutura MQTT

### Tópicos:

```text
sensor/batimentos
sensor/temperatura
```

### Payload:

```json
{
    "bpm": 72
}
```

```json
{
    "temperatura": 24.5
}
```

---

# 📝 Regras de Negócio

- BPM calculado a cada 10 segundos
- Temperatura coletada a cada 10 segundos
- Dados inválidos não são enviados
- Reconexão automática MQTT
- Client ID único por dispositivo
- Comunicação leve (JSON + MQTT)

---

# 📊 Node-RED (Visualização)

## Fluxo básico:

```text
[MQTT IN] → [Function] → [Dashboard]
```

### Function (BPM):

```javascript
msg.payload = msg.payload.bpm;
return msg;
```

### Function (Temperatura):

```javascript
msg.payload = {
    temp: msg.payload.temperatura,
};
return msg;
```

---

# ▶️ Como Executar o Projeto

## 🔧 1. ESP32 (Wokwi)

- Acesse o Wokwi
- Cole o código do ESP32
- Use broker público:

```cpp
mqtt_server = "192.168.0.120";
```

---

## 🧠 2. Node-RED

- Adicione MQTT IN nodes:
    - `sensor/batimentos`
    - `sensor/temperatura`

- Configure broker:

````text
broker.hivemq.com:1883```

---

## ☁️ 3. Azure (opcional)
* Conectar via:
  * IoT Hub
  * Stream Analytics
  * Power BI
---

# 📈 Benefícios

* Monitoramento em tempo real
* Arquitetura escalável IoT
* Baixo custo
* Fácil integração com cloud
* Base para soluções de saúde digital

---

# 🚀 Diferenciais do Projeto

* Uso de arquitetura Edge + Fog + Cloud
* Comunicação eficiente com MQTT
* Estrutura pronta para escalar
* Integração com ferramentas de mercado

---

# 🎤 Roteiro de Apresentação (3–5 minutos)
### 1. Problema
Monitoramento contínuo de sinais vitais de forma acessível.

### 2. Solução
Uso de ESP32 + sensores + MQTT + cloud.

### 3. Arquitetura

Explicar Edge → Fog → Cloud.

### 4. Demonstração
Mostrar Node-RED recebendo dados em tempo real.

### 5. Futuro
Integração com Azure + alertas + mobile.
---
# 🏁 Conclusão
A solução demonstra uma arquitetura moderna de IoT aplicada à saúde, com capacidade de expansão, baixo custo e integração com plataformas de nuvem, atendendo aos requisitos do desafio proposto.
---
````
