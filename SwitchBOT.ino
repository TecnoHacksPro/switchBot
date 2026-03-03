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

const int ANGULO_REPOSO = 0;
const int PASO = 2;            
const int DELAY_MOV = 25;        
const int TIEMPO_PULSO = 150;    

unsigned long lastTimeBotRan;
int botRequestDelay = 1000; 

void moverServoIdaVuelta(int gradosMaximos);
void moverSuave(int desde, int hasta);
void handleNewMessages(int numMessages);

void setup() {
  Serial.begin(115200);

  servo1.setPeriodHertz(50);
  servo1.attach(servoPin);
  servo1.write(ANGULO_REPOSO);
  delay(400);
  servo1.detach();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado.");

  servo1.attach(servoPin);
  moverSuave(0, 15);
  moverSuave(15, 0);
  delay(200);
  servo1.detach();

  Serial.println("Listo y esperando comandos.");
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay) {
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

    if (text.startsWith("pulsar ")) {

      int gradosObjetivo = text.substring(7).toInt();

      if (gradosObjetivo > 60) gradosObjetivo = 60;
      if (gradosObjetivo < 5) gradosObjetivo = 5;

      bot.sendMessage(chat_id, "Pulsando boton suavemente...", "");
      moverServoIdaVuelta(gradosObjetivo);
    }
  }
}

void moverServoIdaVuelta(int gradosMaximos) {
  servo1.attach(servoPin);
  moverSuave(ANGULO_REPOSO, ANGULO_REPOSO);
  delay(100);
  moverSuave(0, gradosMaximos);
  delay(TIEMPO_PULSO);

  moverSuave(gradosMaximos, 0);
  delay(250);  

  servo1.detach();  
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