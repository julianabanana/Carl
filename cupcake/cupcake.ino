/*
  Cupcake Animatrónico — Firmware ESP32

  Dependencias (instalar en Arduino IDE):
    - Adafruit PWM Servo Driver Library  (Adafruit)
    - PubSubClient                        (Nick O'Leary)
    - ArduinoJson                         (Benoit Blanchon)
*/

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"

// ==========================================
// DRIVER DE SERVOS (PCA9685)
// ==========================================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 150
#define SERVOMAX 600

// Canales
const int PARPADOS_SUP = 0;
const int PARPADOS_INF = 1;
const int OJO_IZQ_X   = 2;
const int OJO_IZQ_Y   = 3;
const int OJO_DER_X   = 4;
const int OJO_DER_Y   = 5;
const int BOCA        = 6;

// Límites de seguridad de párpados
const int PARPADOS_SUP_MAX = 10;
const int PARPADOS_SUP_MIN = 90;
const int PARPADOS_INF_MAX = 102;
const int PARPADOS_INF_MIN = 25;

// ==========================================
// MQTT / WIFI
// ==========================================
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// Tópicos — deben coincidir con backend/src/mqtt/topics.js
const char* TOPIC_CMD_SERVO     = "cupcake/commands/servo";
const char* TOPIC_CMD_ANIMATION = "cupcake/commands/animation";
const char* TOPIC_HEARTBEAT     = "cupcake/sensors/heartbeat";

// Intervalo de heartbeat en ms
const unsigned long HEARTBEAT_INTERVAL = 5000;
unsigned long lastHeartbeat = 0;

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);
  abrir();

  connectWiFi();

  mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(256);

  connectMqtt();
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  // Mantener conexiones activas
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected())        connectMqtt();

  mqttClient.loop();
  publishHeartbeat();
}

// ==========================================
// WIFI
// ==========================================
void connectWiFi() {
  Serial.printf("[wifi] Conectando a %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[wifi] Conectado — IP: %s\n", WiFi.localIP().toString().c_str());
}

// ==========================================
// MQTT
// ==========================================
void connectMqtt() {
  while (!mqttClient.connected()) {
    Serial.printf("[mqtt] Conectando a %s:%d ...\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("[mqtt] Conectado al broker");
      mqttClient.subscribe(TOPIC_CMD_SERVO);
      mqttClient.subscribe(TOPIC_CMD_ANIMATION);
    } else {
      Serial.printf("[mqtt] Fallo (rc=%d), reintentando en 3s\n", mqttClient.state());
      delay(3000);
    }
  }
}

// Recibe mensajes del broker y los despacha
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("[mqtt] JSON inválido en %s: %s\n", topic, err.c_str());
    return;
  }

  String topicStr = String(topic);

  if (topicStr == TOPIC_CMD_SERVO) {
    int channel = doc["channel"] | -1;
    int angle   = doc["angle"]   | -1;
    if (channel >= 0 && angle >= 0) {
      moverServo((uint8_t)channel, angle);
    }

  } else if (topicStr == TOPIC_CMD_ANIMATION) {
    const char* id = doc["id"] | "";
    ejecutarAnimacion(id);
  }
}

// Publica uptime y memoria libre cada HEARTBEAT_INTERVAL ms
void publishHeartbeat() {
  if (millis() - lastHeartbeat < HEARTBEAT_INTERVAL) return;
  lastHeartbeat = millis();

  StaticJsonDocument<64> doc;
  doc["uptime"]   = millis();
  doc["freeHeap"] = ESP.getFreeHeap();

  char buffer[64];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_HEARTBEAT, buffer);
}

// ==========================================
// DESPACHADOR DE ANIMACIONES
// ==========================================
void ejecutarAnimacion(const char* id) {
  Serial.printf("[anim] %s\n", id);

  if      (strcmp(id, "abrir")      == 0) abrir();
  else if (strcmp(id, "cerrar")     == 0) cerrar();
  else if (strcmp(id, "pestanear")  == 0) pestanear();
  else if (strcmp(id, "sorpresa")   == 0) sorpresa();
  else if (strcmp(id, "investigar") == 0) investigar();
  else Serial.printf("[anim] Animación desconocida: %s\n", id);
}

// ==========================================
// SERVOS — auxiliar
// ==========================================
void moverServo(uint8_t canal, int angulo) {
  if (canal == PARPADOS_SUP) {
    angulo = constrain(angulo, PARPADOS_SUP_MIN, PARPADOS_SUP_MAX);
  } else if (canal == PARPADOS_INF) {
    angulo = constrain(angulo, PARPADOS_INF_MIN, PARPADOS_INF_MAX);
  } else {
    angulo = constrain(angulo, 0, 180);
  }
  pwm.setPWM(canal, 0, map(angulo, 0, 180, SERVOMIN, SERVOMAX));
}

// ==========================================
// ANIMACIONES
// ==========================================
void cerrar() {
  moverServo(PARPADOS_SUP, PARPADOS_SUP_MAX);
  moverServo(PARPADOS_INF, PARPADOS_INF_MAX);
  moverServo(BOCA, 90);
}

void abrir() {
  moverServo(PARPADOS_SUP, 90);
  moverServo(PARPADOS_INF, 50);
  moverServo(OJO_IZQ_X, 90);
  moverServo(OJO_IZQ_Y, 90);
  moverServo(OJO_DER_X, 90);
  moverServo(OJO_DER_Y, 90);
  moverServo(BOCA, 90);
}

void pestanear() {
  moverServo(PARPADOS_SUP, 10);
  moverServo(PARPADOS_INF, 25);
  delay(150);
  moverServo(PARPADOS_SUP, 90);
  moverServo(PARPADOS_INF, 102);
  delay(150);
  moverServo(PARPADOS_SUP, 10);
  moverServo(PARPADOS_INF, 25);
  delay(150);
  moverServo(PARPADOS_SUP, 90);
  moverServo(PARPADOS_INF, 102);
  abrir();
}

void sorpresa() {
  moverServo(PARPADOS_SUP, PARPADOS_SUP_MIN);
  moverServo(PARPADOS_INF, PARPADOS_INF_MIN);
  moverServo(OJO_IZQ_X, 90);
  moverServo(OJO_IZQ_Y, 90);
  moverServo(OJO_DER_X, 90);
  moverServo(OJO_DER_Y, 90);
  moverServo(BOCA, 150);
  delay(100);
}

void investigar() {
  moverServo(PARPADOS_SUP, 30);
  moverServo(PARPADOS_INF, 85);
  moverServo(BOCA, 100);
  moverServo(OJO_IZQ_X, 45);
  moverServo(OJO_DER_X, 45);
  delay(1500);
  moverServo(OJO_IZQ_X, 135);
  moverServo(OJO_DER_X, 135);
  delay(1500);
  abrir();
}
