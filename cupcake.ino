#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Crear el objeto PCA9685 en la dirección I2C por defecto (0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Valores PWM para 0 y 180 grados.
#define SERVOMIN  150 
#define SERVOMAX  600 

// ==========================================
// NUEVA DEFINICIÓN DE CANALES (7 Servomotores)
// ==========================================
const int PARPADOS_SUP = 0; // Controla ambos párpados superiores
const int PARPADOS_INF = 1; // Controla ambos párpados inferiores
const int OJO_IZQ_X    = 2; // Ojo izquierdo (Izquierda / Derecha)
const int OJO_IZQ_Y    = 3; // Ojo izquierdo (Arriba / Abajo)
const int OJO_DER_X    = 4; // Ojo derecho (Izquierda / Derecha)
const int OJO_DER_Y    = 5; // Ojo derecho (Arriba / Abajo)
const int BOCA         = 6; // Servo MG995 para la mandíbula

// Límites de seguridad para los párpados
const int PARPADOS_SUP_MAX = 10;
const int PARPADOS_SUP_MIN = 90;
const int PARPADOS_INF_MAX = 102;
const int PARPADOS_INF_MIN = 25;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Animatrónico...");

  // Si usaste el truco de cambiar pines (ej: Wire.begin(16, 17);), agrégalo aquí.
  // Si te funciona sin eso, déjalo así:
  pwm.begin();
  pwm.setPWMFreq(50); 
  delay(10);

  // Posición inicial: Despierto y boca cerrada
  abrir();

  Serial.println("--------------------------------------------------");
  Serial.println("MODO CALIBRACIÓN: ACTIVO");
  Serial.println("Envía un número de 4 dígitos para mover un servo.");
  Serial.println("Formato: SAAA (S = Canal del servo, AAA = Ángulo)");
  Serial.println("Ejemplo: '0090' mueve el servo 0 a 90 grados.");
  Serial.println("Ejemplo: '6180' mueve el servo 6 a 180 grados.");
  Serial.println("--------------------------------------------------");
}

void loop() {
  // 1. Escuchar comandos desde el Monitor Serie
  /*if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim(); // Limpiar espacios ocultos o saltos de línea

    if (comando.length() == 4) {
      // Extraer el canal (primer dígito) y el ángulo (últimos 3 dígitos)
      int canal = comando.substring(0, 1).toInt();
      int angulo = comando.substring(1, 4).toInt();

      Serial.print("Calibrando -> Canal: ");
      Serial.print(canal);
      Serial.print(" | Ángulo: ");
      Serial.println(angulo);

      // Ejecutar el movimiento
      moverServo(canal, angulo);
    } 
    else if (comando.length() > 0) {
      Serial.println("Error: Formato incorrecto. Usa 4 dígitos exactos (ej: '0090').");
    }
  }*/

  // =================================================================
  // SECUENCIA AUTOMÁTICA (COMENTADA PARA CALIBRAR)
  // Cuando termines de calibrar y ajustar tus valores en las funciones 
  // de abajo, borra el /* de arriba y el */ /* de abajo para reactivarla.
  // =================================================================
  
  //Serial.println("Ejecutando: Pestañear");
  //pestanear();
  //delay(2000);

  //Serial.println("Ejecutando: Sorpresa");
  //sorpresa();
  //delay(3000);

  Serial.println("Ejecutando: Cerrar");
  cerrar();
  delay(3000);

  Serial.println("Ejecutando: Abrir");
  abrir();
  delay(3000);
  
}

// ==========================================
// FUNCIÓN AUXILIAR: Convierte grados a pulsos
// ==========================================
void moverServo(uint8_t canal, int angulo) {
  // Aplicar límites de seguridad rígidos si son los párpados
  if (canal == PARPADOS_SUP) {
    angulo = constrain(angulo, PARPADOS_SUP_MIN, PARPADOS_SUP_MAX);
  } else if (canal == PARPADOS_INF) {
    angulo = constrain(angulo, PARPADOS_INF_MIN, PARPADOS_INF_MAX);
  } else {
    // Para los demás servos, el límite estándar es de 0 a 180
    angulo = constrain(angulo, 0, 180);
  }
  
  int pulso = map(angulo, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(canal, 0, pulso);
}

// ==========================================
// FUNCIONES DEL ANIMATRÓNICO
// ==========================================
// IMPORTANTE: Los ángulos (0, 90, 180) son referencias. 
// Deberás calibrarlos según cómo armaste los engranajes o varillas.

void cerrar() {
  // Bajar párpados superiores, subir párpados inferiores (Límites cerrados)
  moverServo(PARPADOS_SUP, PARPADOS_SUP_MAX); // 10
  moverServo(PARPADOS_INF, PARPADOS_INF_MAX); // 102
  
  // Mantener boca cerrada
  moverServo(BOCA, 90); 
}

void abrir() {
  // Párpados en posición neutral (ojos abiertos normales, intermedios)
  moverServo(PARPADOS_SUP, 90); 
  moverServo(PARPADOS_INF, 50);
  
  // Ojos centrados mirando al frente
  moverServo(OJO_IZQ_X, 90);
  moverServo(OJO_IZQ_Y, 90);
  moverServo(OJO_DER_X, 90);
  moverServo(OJO_DER_Y, 90);
  
  // Boca cerrada
  moverServo(BOCA, 90);
}

void pestanear() {
  moverServo(PARPADOS_SUP,10);
  moverServo(PARPADOS_INF,25);
  delay(150); // Tiempo que el ojo está cerrado (rápido)
  moverServo(PARPADOS_SUP,90);
  moverServo(PARPADOS_INF,102);
  delay(150);
  moverServo(PARPADOS_SUP,10);
  moverServo(PARPADOS_INF,25);
  delay(150); // Tiempo que el ojo está cerrado (rápido)
  moverServo(PARPADOS_SUP,90);
  moverServo(PARPADOS_INF,102);
  abrir();
}

void sorpresa() {
  // Ojos pelados (abiertos al máximo permitido por los límites)
  moverServo(PARPADOS_SUP, PARPADOS_SUP_MIN); // 90
  moverServo(PARPADOS_INF, PARPADOS_INF_MIN); // 25
  
  // Ojos centrados
  moverServo(OJO_IZQ_X, 90);
  moverServo(OJO_IZQ_Y, 90);
  moverServo(OJO_DER_X, 90);
  moverServo(OJO_DER_Y, 90);

  // ¡La mandíbula cae abierta! (MG995)
  moverServo(BOCA, 150); // Ajustar para apertura máxima de boca
  delay(100); 
}

void investigar() {
  // Entrecerrar los ojos (actitud sospechosa, cerca de los límites de cierre)
  moverServo(PARPADOS_SUP, 30);  
  moverServo(PARPADOS_INF, 85); 
  
  // Boca ligeramente abierta o apretada
  moverServo(BOCA, 100);

  // Ambos ojos miran a la izquierda
  // (Si un ojo se mueve al revés, invierte su valor, ej: cambia 45 por 135)
  moverServo(OJO_IZQ_X, 45); 
  moverServo(OJO_DER_X, 45);
  delay(1500);

  // Ambos ojos miran a la derecha
  moverServo(OJO_IZQ_X, 135);
  moverServo(OJO_DER_X, 135);
  delay(1500);

  // Volver a la normalidad
  abrir();
}