#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define BOT_TOKEN ""
#define CHAT_ID ""

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

static const int servoPin = 13;
Servo servo1;

// ---- Configuración del movimiento ----
const int ANGULO_REPOSO   = 0;
const int ANGULO_PULSO_DEFAULT = 28;   // Ángulo por defecto al arrancar
const int PASO            = 2;
const int DELAY_MOV       = 25;
const int TIEMPO_PULSO    = 150;       // Tiempo manteniendo presionado
const int TIEMPO_ASENTAR  = 350;       // Tiempo extra antes de detach para que el servo se quede quieto

unsigned long lastTimeBotRan;
const int botRequestDelay = 1000;

// ---- Prototipos ----
void pulsarBoton(int gradosMaximos);
void moverSuave(int desde, int hasta);
void handleNewMessages(int numMessages);
void liberarServo();
void engancharServo();

void setup() {
  Serial.begin(115200);

  // Aseguramos pin en LOW antes de nada para evitar pulsos basura
  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, LOW);

  // Conexión WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Pulsación inicial automática tras conectar
  Serial.println("Pulsacion inicial...");
  pulsarBoton(ANGULO_PULSO_DEFAULT);

  Serial.println("Listo y esperando comandos.");
}

void loop() {
  if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}

void handleNewMessages(int numMessages) {
  for (int i = 0; i < numMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Usuario no autorizado", "");
      continue;
    }

    text.toLowerCase();

    if (text.startsWith("pulsar")) {
      int gradosObjetivo = ANGULO_PULSO_DEFAULT;

      // Si viene con número detrás, lo usamos
      if (text.length() > 7) {
        int g = text.substring(7).toInt();
        if (g > 0) gradosObjetivo = g;
      }

      if (gradosObjetivo > 60) gradosObjetivo = 60;
      if (gradosObjetivo < 5)  gradosObjetivo = 5;

      bot.sendMessage(chat_id, "Pulsando boton a " + String(gradosObjetivo) + "...", "");
      pulsarBoton(gradosObjetivo);
      bot.sendMessage(chat_id, "Listo ✅", "");
    }
  }
}

// ---- Lógica del servo ----

void engancharServo() {
  servo1.setPeriodHertz(50);
  // Rango de pulsos típico SG90: 500-2400 us
  servo1.attach(servoPin, 500, 2400);
}

void liberarServo() {
  // Detach + forzamos pin a LOW para eliminar cualquier pulso residual
  // que pueda provocar micro-movimientos / jitter del SG90.
  servo1.detach();
  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, LOW);
}

void pulsarBoton(int gradosMaximos) {
  engancharServo();
  servo1.write(ANGULO_REPOSO);
  delay(150);

  moverSuave(ANGULO_REPOSO, gradosMaximos);
  delay(TIEMPO_PULSO);

  moverSuave(gradosMaximos, ANGULO_REPOSO);

  // Importante: esperar a que el servo termine físicamente
  // antes de cortar la señal, si no a veces se queda a medias
  // y luego "tiembla" cuando vuelves a engancharlo.
  delay(TIEMPO_ASENTAR);

  liberarServo();
}

void moverSuave(int desde, int hasta) {
  if (desde < hasta) {
    for (int pos = desde; pos <= hasta; pos += PASO) {
      servo1.write(pos);
      delay(DELAY_MOV);
    }
  } else {
    for (int pos = desde; pos >= hasta; pos -= PASO) {
      servo1.write(pos);
      delay(DELAY_MOV);
    }
  }
  servo1.write(hasta);
}
