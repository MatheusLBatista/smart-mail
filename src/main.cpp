#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ─── Configurações ────────────────────────────────────────────────────────────
const char* WIFI_SSID     = "SEU_WIFI_AQUI";
const char* WIFI_PASSWORD = "SUA_SENHA_AQUI";

const int PINO_SENSOR = 19;
const int PINO_LED    = 18;

const unsigned long DEBOUNCE_MS = 15000;
// ─────────────────────────────────────────────────────────────────────────────

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool          ultimoEstado  = HIGH;
unsigned long ultimoGatilho = 0;

// ─── WebSocket ────────────────────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
               void*, uint8_t*, size_t) {
  if (type == WS_EVT_CONNECT)
    Serial.printf("[WS] Cliente conectado: #%u\n", client->id());
  else if (type == WS_EVT_DISCONNECT)
    Serial.printf("[WS] Cliente desconectado: #%u\n", client->id());
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
String getTime() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "--:--:--";
  char buf[10];
  strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
  return String(buf);
}

String getDate() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "";
  char buf[12];
  strftime(buf, sizeof(buf), "%d/%m/%Y", &ti);
  return String(buf);
}

void dispararEvento() {
  String t   = getTime();
  String d   = getDate();
  String msg = "{\"type\":\"mail\",\"time\":\"" + t + "\",\"date\":\"" + d + "\"}";
  ws.textAll(msg);
  Serial.printf("[SENSOR] Correio detectado! %s %s\n", t.c_str(), d.c_str());
}

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(PINO_SENSOR, INPUT);
  pinMode(PINO_LED, OUTPUT);

  if (!LittleFS.begin(true))
    Serial.println("[ERRO] Falha ao montar LittleFS");

  Serial.printf("\nConectando ao WiFi: %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());

  // NTP — fuso horário de Brasília (UTC-3)
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.begin();
  Serial.println("Acesse: http://" + WiFi.localIP().toString());
}

// ─── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
  bool          estadoAtual = digitalRead(PINO_SENSOR);
  unsigned long agora       = millis();

  // Borda de descida (HIGH → LOW) = correio inserido
  if (ultimoEstado == HIGH && estadoAtual == LOW) {
    if (agora - ultimoGatilho > DEBOUNCE_MS) {
      ultimoGatilho = agora;
      digitalWrite(PINO_LED, HIGH);
      dispararEvento();
    }
  }

  if (estadoAtual == HIGH)
    digitalWrite(PINO_LED, LOW);

  ultimoEstado = estadoAtual;
  ws.cleanupClients();
  delay(50);
}
