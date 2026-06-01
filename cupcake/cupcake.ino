/*
 * ═══════════════════════════════════════════════════════════════════
 *  C A R L  —  Mascota Robótica
 *  ESP32 + PCA9685 + DFPlayer Mini + HC-SR04 + DHT22 + NeoPixel
 * ═══════════════════════════════════════════════════════════════════
 *
 *  MAPA DE CANALES PCA9685:
 *   Canal 4  → Parpado superior   (70° abierto / 145° cerrado)
 *   Canal 5  → Parpado inferior   (70° abierto / 145° cerrado)
 *   Canal 6  → Ojo izquierdo X    (40°–140°)
 *   Canal 7  → Ojo derecho  X     (40°–140°)
 *   Canal 8  → Ojo izquierdo Y    (90°–120°)
 *   Canal 9  → Ojo derecho  Y     (55°–105°)
 *   Canal 10 → Boca              (70° cerrado / 120° abierto)
 *
 *  PINES ESP32:
 *   GPIO 21 / 22 → I2C (SDA/SCL) para PCA9685
 *   GPIO 16      → DFPlayer TX (RX del módulo)
 *   GPIO 17      → DFPlayer RX (TX del módulo)
 *   GPIO 18      → HC-SR04 TRIG
 *   GPIO 19      → HC-SR04 ECHO
 *   GPIO 4       → DHT22 DATA
 *   GPIO 5       → NeoPixel DATA (ambos aros)
 *   GPIO 13      → LED indicador de función activa
 */

// ─── Librerías ───────────────────────────────────────────────────────────────
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include <time.h>

// ─── Pines ───────────────────────────────────────────────────────────────────
#define PIN_TRIG      18
#define PIN_ECHO      19
#define PIN_DHT       4
#define PIN_NEO       5
#define PIN_LED       13   // LED indicador de función activa
#define NUM_LEDS      16   // 2 aros x 8 LEDs

// ─── Servos: rangos (°) ──────────────────────────────────────────────────────
#define PARP_SUP    4
#define PARP_INF    5
#define OJO_IZQ_X  6
#define OJO_DER_X  7
#define OJO_IZQ_Y  8
#define OJO_DER_Y  9
#define BOCA       10

// Posiciones de referencia
const int POS_PARP_ABIERTO  = 70;
const int POS_PARP_CERRADO  = 145;
const int POS_PARP_SOSPECHA = 120;
const int POS_PARP_SORPRESA = 55;
const int POS_BOCA_ABIERTA  = 110;
const int POS_BOCA_CERRADA  = 90;

// Rangos de movimiento ojos
const int OJO_X_MIN = 40,  OJO_X_MAX = 140, OJO_X_CEN = 90;
const int OJO_IZQ_Y_MIN = 90, OJO_IZQ_Y_MAX = 120, OJO_IZQ_Y_CEN = 90;
const int OJO_DER_Y_MIN = 55, OJO_DER_Y_MAX = 105, OJO_DER_Y_CEN = 105;

// ─── Velocidades ─────────────────────────────────────────────────────────────
#define VEL_LENTA   2
#define VEL_MEDIA   6
#define VEL_RAPIDA  8

// ─── WiFi ────────────────────────────────────────────────────────────────────
const char* SSID_AP = "Cupcake-Bot";
const char* PASS_AP = "mrcupcake1234";

// ─── NTP ─────────────────────────────────────────────────────────────────────
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET = -18000;   // UTC-5 (Colombia)
const int   DST_OFFSET = 0;

// ─── Objetos globales ─────────────────────────────────────────────────────────
Adafruit_PWMServoDriver pca(0x40);
WebServer               server(80);
Adafruit_NeoPixel       strip(NUM_LEDS, PIN_NEO, NEO_GRB + NEO_KHZ800);
DHT                     dht(PIN_DHT, DHT22);
HardwareSerial          dfSerial(2);  // UART2 del ESP32
DFRobotDFPlayerMini     player;

// ─── Estado del sistema ───────────────────────────────────────────────────────
int  posActual[16];           // Posiciones actuales de servos
float ultimaTemp = -99.0f;
float ultimaHum  = -99.0f;

// Temporizadores automáticos (ms)
unsigned long tInvestigar = 0;
unsigned long tTemperatura = 0;
const unsigned long INTERVALO_AUTO = 1800000UL;  // 30 minutos
bool investAuto = false;
bool tempAuto   = false;
bool funcEnCurso = false;  // Bloqueo de reentrada

// ─── Alarma ──────────────────────────────────────────────────────────────────
int  alarmaHora  = -1;
int  alarmaMin   = -1;
bool alarmaActiva = false;

// ═══════════════════════════════════════════════════════════════════
//  SECCIÓN: LED INDICADOR (pin 13)
// ═══════════════════════════════════════════════════════════════════
void ledIndicadorOn()  { digitalWrite(PIN_LED, HIGH); }
void ledIndicadorOff() { digitalWrite(PIN_LED, LOW);  }

// ═══════════════════════════════════════════════════════════════════
//  SECCIÓN: SERVOS
// ═══════════════════════════════════════════════════════════════════

int _anguloAMicros(int canal, int ang) {
    ang = constrain(ang, 0, 180);
    int pmin = (canal == BOCA) ? 500 : 500;   // MG995 y SG90 mismo min
    int pmax = (canal == BOCA) ? 2500 : 2400;
    return map(ang, 0, 180, pmin, pmax);
}

void _aplicar(int canal, int ang) {
    ang = constrain(ang, 0, 180);
    int ticks = (int)((float)_anguloAMicros(canal, ang) * 4096.0f / 20000.0f);
    pca.setPWM(canal, 0, ticks);
    posActual[canal] = ang;
}

void moverServo(int canal, int dest, int vel) {
    vel  = constrain(vel, 1, 10);
    dest = constrain(dest, 0, 180);
    int ori  = posActual[canal];
    if (ori == dest) return;
    int ms   = map(vel, 1, 10, 30, 2);
    int paso = (dest > ori) ? 1 : -1;
    for (int p = ori; p != dest; p += paso) { _aplicar(canal, p); delay(ms); }
    _aplicar(canal, dest);
}

void moverSimultaneo(int* canales, int* dests, int n, int vel) {
    vel = constrain(vel, 1, 10);
    n   = constrain(n, 1, 8);
    int ms = map(vel, 1, 10, 30, 2);
    int ori[8], maxP = 0;
    for (int i = 0; i < n; i++) { ori[i] = posActual[canales[i]]; }
    for (int i = 0; i < n; i++) { int d = abs(dests[i]-ori[i]); if(d>maxP) maxP=d; }
    if (!maxP) return;
    for (int s = 1; s <= maxP; s++) {
        for (int i = 0; i < n; i++) {
            int a = ori[i] + (int)roundf((float)(dests[i]-ori[i])*s/maxP);
            _aplicar(canales[i], a);
        }
        delay(ms);
    }
    for (int i = 0; i < n; i++) _aplicar(canales[i], dests[i]);
}

// ─── Primitivas de parpados / ojos / boca ─────────────────────────────────────
void parpadoNormal() {
    int c[]={PARP_SUP,PARP_INF}, a[]={POS_PARP_ABIERTO,POS_PARP_ABIERTO};
    moverSimultaneo(c,a,2,VEL_MEDIA);
}
void parpadoCerrado() {
    int c[]={PARP_SUP,PARP_INF}, a[]={POS_PARP_CERRADO,POS_PARP_CERRADO};
    moverSimultaneo(c,a,2,VEL_MEDIA);
}
void bocaAbrir() { moverServo(BOCA, POS_BOCA_ABIERTA, VEL_MEDIA); }
void bocaCerrar(){ moverServo(BOCA, POS_BOCA_CERRADA, VEL_MEDIA); }
void ojosAlCentro() {
    int c[]={OJO_IZQ_X,OJO_DER_X,OJO_IZQ_Y,OJO_DER_Y};
    int a[]={OJO_X_CEN, OJO_X_CEN, OJO_IZQ_Y_CEN, OJO_DER_Y_CEN};
    moverSimultaneo(c,a,4,VEL_MEDIA);
}

// ─── Distancia (HC-SR04) ─────────────────────────────────────────────────────
float medirDistancia() {
    digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    long dur = pulseIn(PIN_ECHO, HIGH, 30000);
    return (dur == 0) ? 999.0f : dur * 0.0343f / 2.0f;
}

// ═══════════════════════════════════════════════════════════════════
//  SECCIÓN: NEOPIXEL
// ═══════════════════════════════════════════════════════════════════
void ledColor(uint32_t color) { strip.fill(color); strip.show(); }
void ledApagar()               { strip.clear(); strip.show(); }

void ledRainbow() {
    for (int j = 0; j < 256; j += 8) {
        for (int i = 0; i < NUM_LEDS; i++)
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((i*65536L/NUM_LEDS + j*256))));
        strip.show();
        delay(20);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  SECCIÓN: ACCIONES (botones de la interfaz)
// ═══════════════════════════════════════════════════════════════════

void accionCerrar() {
    ledIndicadorOn();
    parpadoCerrado();
    ledIndicadorOff();
}

void accionSorpresa() {
    ledIndicadorOn();
    int c[]={PARP_SUP,PARP_INF}, a[]={POS_PARP_SORPRESA,POS_PARP_SORPRESA};
    moverSimultaneo(c,a,2,VEL_MEDIA);
    delay(1000);
    parpadoNormal();
    ledIndicadorOff();
}

void accionPestanear() {
    ledIndicadorOn();
    int c[]={PARP_SUP,PARP_INF};
    int cerr[]={POS_PARP_CERRADO,POS_PARP_CERRADO};
    int abier[]={POS_PARP_ABIERTO,POS_PARP_ABIERTO};
    moverSimultaneo(c,cerr,2,VEL_RAPIDA); delay(80);
    moverSimultaneo(c,abier,2,VEL_RAPIDA);
    ledIndicadorOff();
}

void accionSospechar() {
    ledIndicadorOn();
    int c[]={PARP_SUP,PARP_INF}, a[]={POS_PARP_SOSPECHA,POS_PARP_SOSPECHA};
    moverSimultaneo(c,a,2,VEL_MEDIA);
    delay(2000);
    parpadoNormal();
    ledIndicadorOff();
}

void accionLados() {
    ledIndicadorOn();
    int c[]={OJO_IZQ_X,OJO_DER_X};
    int der[]={OJO_X_MAX,OJO_X_MAX}, izq[]={OJO_X_MIN,OJO_X_MIN}, cen[]={OJO_X_CEN,OJO_X_CEN};
    moverSimultaneo(c,der,2,VEL_MEDIA); delay(400);
    moverSimultaneo(c,izq,2,VEL_MEDIA); delay(400);
    moverSimultaneo(c,cen,2,VEL_MEDIA);
    ledIndicadorOff();
}

void accionOjosLocos() {
    ledIndicadorOn();
    int c[]={OJO_IZQ_X,OJO_DER_X,OJO_IZQ_Y,OJO_DER_Y};
    for (int i = 0; i < 8; i++) {
        int a[]={
            random(OJO_X_MIN,OJO_X_MAX),
            random(OJO_X_MIN,OJO_X_MAX),
            random(OJO_IZQ_Y_MIN,OJO_IZQ_Y_MAX),
            random(OJO_DER_Y_MIN,OJO_DER_Y_MAX)
        };
        moverSimultaneo(c,a,4,VEL_RAPIDA);
        delay(200);
    }
    ojosAlCentro();
    ledIndicadorOff();
}

void accionSaludar() {
    ledIndicadorOn();
    bocaAbrir();
    player.play(1);  // 001.mp3
    delay(3000);
    bocaCerrar();
    ledIndicadorOff();
}

void accionDespedir() {
    ledIndicadorOn();
    bocaAbrir();
    player.play(2);  // 002.mp3
    delay(3000);
    bocaCerrar();
    ledIndicadorOff();
}

// ═══════════════════════════════════════════════════════════════════
//  SECCIÓN: FUNCIONES AUTOMÁTICAS
// ═══════════════════════════════════════════════════════════════════

// [CAMBIO 3] Investigar: primero audio, luego ojos rojos
void funcInvestigar() {
    if (funcEnCurso) return;
    funcEnCurso = true;
    ledIndicadorOn();

    // Primero el audio
    player.play(3);   // 003.mp3
    Serial.println("Audio 3");

    // Luego ojos naranja y sospecha
    ledColor(strip.Color(255, 50, 0));
    int c[]={PARP_SUP,PARP_INF}, s[]={POS_PARP_SOSPECHA,POS_PARP_SOSPECHA};
    moverSimultaneo(c,s,2,VEL_MEDIA);

    // Mira lados
    accionLados();
    delay(500);

    // Mide distancia
    bocaAbrir();
    float dist = medirDistancia();
    Serial.println(dist);

    if (dist < 20.0f) {
        // Primero audio, luego ojos rojos
        player.play(4);  // 004.mp3
        ledColor(strip.Color(255, 0, 0));
        int sorpr[]={POS_PARP_SORPRESA,POS_PARP_SORPRESA};
        moverSimultaneo(c,sorpr,2,VEL_MEDIA);
        delay(3000);
    }

    // Restablecer
    ledApagar();
    bocaCerrar();
    parpadoNormal();
    ledIndicadorOff();
    funcEnCurso = false;
}

void funcTemperatura() {
    if (funcEnCurso) return;
    funcEnCurso = true;
    ledIndicadorOn();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) ultimaTemp = t;
    if (!isnan(h)) ultimaHum  = h;

    if (t >= 15.0f && t <= 25.0f) { ledIndicadorOff(); funcEnCurso = false; return; }

    bocaAbrir();

    if (t < 15.0f) {
        ledColor(strip.Color(0, 80, 255));
        moverServo(PARP_INF, 140, VEL_MEDIA);
        player.play(5);  // 005.mp3
    } else {
        ledColor(strip.Color(255, 0, 0));
        moverServo(PARP_SUP, 140, VEL_MEDIA);
        player.play(6);  // 006.mp3
    }
    delay(3000);

    ledApagar();
    bocaCerrar();
    parpadoNormal();
    ledIndicadorOff();
    funcEnCurso = false;
}

// [CAMBIO 2] Fiesta: LEDs encendidos durante toda la reproducción del MP3
void funcFiesta() {
    if (funcEnCurso) return;
    funcEnCurso = true;
    ledIndicadorOn();

    bocaAbrir();
    int pista = random(7, 13);  // 007.mp3 – 012.mp3
    player.play(pista);

    int c[]={OJO_IZQ_X,OJO_DER_X,OJO_IZQ_Y,OJO_DER_Y};
    uint32_t colores[] = {
        strip.Color(255,0,100), strip.Color(0,255,180),
        strip.Color(255,200,0), strip.Color(0,100,255),
        strip.Color(200,0,255)
    };

    // Animar LEDs durante toda la reproducción (máx 120s)
    unsigned long inicio = millis();
    int ronda = 0;
    while (millis() - inicio < 120000UL) {
        ledColor(colores[ronda % 5]);
        int a[]={
            random(OJO_X_MIN,OJO_X_MAX), random(OJO_X_MIN,OJO_X_MAX),
            random(OJO_IZQ_Y_MIN,OJO_IZQ_Y_MAX), random(OJO_DER_Y_MIN,OJO_DER_Y_MAX)
        };
        moverSimultaneo(c,a,4,VEL_RAPIDA);
        delay(300);
        ledRainbow();
        ronda++;
        // Salir si el DFPlayer terminó la pista
        if (player.available()) {
            if (player.readType() == DFPlayerPlayFinished) break;
        }
    }

    ojosAlCentro();
    bocaCerrar();
    ledApagar();
    ledIndicadorOff();
    funcEnCurso = false;
}

// [CAMBIO 6] Alarma → fiesta con detección ultrasónica para apagar
void funcAlarmaFiesta() {
    if (funcEnCurso) return;
    funcEnCurso = true;
    ledIndicadorOn();

    bocaAbrir();
    int pista = random(7, 13);
    player.play(pista);

    int c[]={OJO_IZQ_X,OJO_DER_X,OJO_IZQ_Y,OJO_DER_Y};
    uint32_t colores[] = {
        strip.Color(255,0,100), strip.Color(0,255,180),
        strip.Color(255,200,0), strip.Color(0,100,255),
        strip.Color(200,0,255)
    };

    unsigned long inicio = millis();
    int ronda = 0;
    bool detenida = false;
    while (millis() - inicio < 120000UL) {
        // Revisar sensor ultrasónico: apagar si < 5 cm
        float dist = medirDistancia();
        if (dist < 5.0f) {
            detenida = true;
            break;
        }
        ledColor(colores[ronda % 5]);
        int a[]={
            random(OJO_X_MIN,OJO_X_MAX), random(OJO_X_MIN,OJO_X_MAX),
            random(OJO_IZQ_Y_MIN,OJO_IZQ_Y_MAX), random(OJO_DER_Y_MIN,OJO_DER_Y_MAX)
        };
        moverSimultaneo(c,a,4,VEL_RAPIDA);
        delay(300);
        ledRainbow();
        ronda++;
        if (player.available()) {
            if (player.readType() == DFPlayerPlayFinished) break;
        }
    }

    if (detenida) player.stop();

    ojosAlCentro();
    bocaCerrar();
    ledApagar();
    ledIndicadorOff();
    funcEnCurso = false;
}

// ═══════════════════════════════════════════════════════════════════
//  SECCIÓN: INTERFAZ WEB
// ═══════════════════════════════════════════════════════════════════

// Tiempo restante (segundos) para cada función automática
long segParaInvestigar() { return max(0L, (long)((tInvestigar - millis())/1000)); }
long segParaTemperatura() { return max(0L, (long)((tTemperatura - millis())/1000)); }

String formatearTiempo(long s) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02ld:%02ld", s/60, s%60);
    return String(buf);
}

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="es"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Mr. Cupcake's controls</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Press+Start+2P&family=VT323:wght@400&display=swap');
:root{
  --morado:#9B30FF;--morado2:#7B2FBE;--rosa:#FF2D78;--negro:#050510;
  --rosa2:#FF69B4;--gris:#1A1A2E;--morado-claro:#BF7FFF;
}
*{box-sizing:border-box;margin:0;padding:0;}
body{background:var(--negro);color:var(--morado);font-family:'Press Start 2P',monospace;min-height:100vh;
  background-image:radial-gradient(circle at 20% 50%,rgba(155,48,255,0.1) 0%,transparent 50%),
  radial-gradient(circle at 80% 20%,rgba(123,47,190,0.1) 0%,transparent 50%);}
header{text-align:center;padding:28px 16px 12px;border-bottom:2px solid var(--morado);}
header h1{font-size:clamp(16px,5vw,32px);color:var(--morado);text-shadow:0 0 12px var(--morado),0 0 30px var(--morado2);
  letter-spacing:4px;animation:pulso 2s ease-in-out infinite;}
header p{font-size:10px;color:var(--morado-claro);margin-top:10px;letter-spacing:2px;}
@keyframes pulso{0%,100%{opacity:1;}50%{opacity:.7;}}

.status{display:grid;grid-template-columns:1fr 1fr;gap:12px;padding:16px;max-width:700px;margin:0 auto;}
.stat-box{background:var(--gris);border:2px solid var(--morado2);padding:14px;border-radius:4px;}
.stat-box h3{font-size:9px;color:var(--rosa);margin-bottom:8px;letter-spacing:1px;}
.stat-val{font-family:'VT323',monospace;font-size:32px;color:var(--morado);}
.countdown{font-family:'VT323',monospace;font-size:26px;color:var(--rosa2);}
.label-sm{font-size:8px;color:#888;margin-top:4px;}

.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));
  gap:12px;padding:16px;max-width:750px;margin:0 auto;}

.btn{background:var(--negro);border:2px solid var(--morado);color:var(--morado);
  font-family:'Press Start 2P',monospace;font-size:9px;padding:14px 8px;cursor:pointer;
  border-radius:4px;text-align:center;line-height:1.7;transition:all .15s;
  text-shadow:0 0 6px var(--morado);position:relative;overflow:hidden;}
.btn:hover{background:var(--morado);color:var(--negro);text-shadow:none;
  box-shadow:0 0 18px var(--morado),0 0 40px rgba(155,48,255,.3);}
.btn:active{transform:scale(.96);}
.btn.rosa{border-color:var(--rosa);color:var(--rosa);text-shadow:0 0 6px var(--rosa);}
.btn.rosa:hover{background:var(--rosa);color:var(--negro);}
.btn.fiesta{border-color:#FFD700;color:#FFD700;font-size:10px;}
.btn.fiesta:hover{background:#FFD700;color:var(--negro);box-shadow:0 0 24px gold;}
.btn .ico{font-size:22px;display:block;margin-bottom:6px;}

/* ── Sección de prueba de servos ─────────────────────────── */
.seccion{max-width:750px;margin:16px auto;padding:0 16px;}
.seccion h2{font-size:11px;color:var(--morado);margin-bottom:12px;text-shadow:0 0 8px var(--morado);}
.panel{background:var(--gris);border:2px solid var(--morado2);padding:16px;border-radius:4px;
  display:flex;flex-wrap:wrap;gap:12px;align-items:flex-end;}
.campo{display:flex;flex-direction:column;gap:4px;}
.campo label{font-size:8px;color:var(--morado-claro);}
.campo input,.campo select{background:var(--negro);border:2px solid var(--morado2);color:var(--morado);
  font-family:'VT323',monospace;font-size:22px;padding:6px 10px;border-radius:2px;width:100px;text-align:center;}
.campo input:focus,.campo select:focus{outline:none;border-color:var(--morado);box-shadow:0 0 8px var(--morado);}
.btn-sm{background:var(--morado);color:var(--negro);font-family:'Press Start 2P',monospace;
  font-size:9px;padding:10px 16px;border:none;border-radius:4px;cursor:pointer;transition:all .15s;}
.btn-sm:hover{box-shadow:0 0 14px var(--morado);transform:scale(1.03);}
.btn-sm.rojo{background:#FF4444;}
.btn-sm.rojo:hover{box-shadow:0 0 14px #FF4444;}

/* ── Alarma ──────────────────────────────────────────────── */
.alarma-estado{font-family:'VT323',monospace;font-size:26px;color:var(--morado);
  margin-left:12px;align-self:center;}

.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);
  background:var(--gris);border:2px solid var(--morado);color:var(--morado);
  font-family:'Press Start 2P',monospace;font-size:9px;padding:12px 20px;
  border-radius:4px;opacity:0;transition:opacity .3s;pointer-events:none;z-index:99;}
.toast.show{opacity:1;}
footer{text-align:center;font-size:7px;color:#444;padding:28px;letter-spacing:2px;}
</style></head><body>

<header>
  <h1>★ M R.  C U P C A K E ★</h1>
  <p>MASCOTA ROBOT v2.0 // PANEL DE CONTROL</p>
</header>

<div class="status">
  <div class="stat-box">
    <h3>🌡 TEMPERATURA</h3>
    <div class="stat-val" id="temp">--°C</div>
    <div class="stat-val" id="hum" style="font-size:22px;">-- %</div>
    <div class="label-sm">ÚLTIMA LECTURA DHT22</div>
  </div>
  <div class="stat-box">
    <h3>⏱ PRÓXIMAS FUNCIONES</h3>
    <div class="label-sm">🔍 INVESTIGAR</div>
    <div class="countdown" id="cdInv">--:--</div>
    <div class="label-sm" style="margin-top:8px;">🌡 TEMPERATURA</div>
    <div class="countdown" id="cdTemp">--:--</div>
  </div>
</div>

<div class="grid">
  <button class="btn" onclick="cmd('cerrar')"><span class="ico">😑</span>CERRAR</button>
  <button class="btn rosa" onclick="cmd('sorpresa')"><span class="ico">😲</span>SORPRESA</button>
  <button class="btn" onclick="cmd('pestanear')"><span class="ico">😉</span>PESTAÑEAR</button>
  <button class="btn rosa" onclick="cmd('sospechar')"><span class="ico">🤨</span>SOSPECHAR</button>
  <button class="btn" onclick="cmd('lados')"><span class="ico">👀</span>LADOS</button>
  <button class="btn rosa" onclick="cmd('ojoslocos')"><span class="ico">🌀</span>OJOS LOCOS</button>
  <button class="btn" onclick="cmd('saludar')"><span class="ico">👋</span>SALUDAR</button>
  <button class="btn rosa" onclick="cmd('despedir')"><span class="ico">🫡</span>DESPEDIR</button>
  <button class="btn" onclick="cmd('temperatura')"><span class="ico">🌡</span>TEMPERATURA</button>
  <button class="btn rosa" onclick="cmd('investigar')"><span class="ico">🔍</span>INVESTIGAR</button>
  <button class="btn fiesta" onclick="cmd('fiesta')"><span class="ico">🎉</span>FIESTA!</button>
</div>

<!-- SECCIÓN: PRUEBA DE SERVOS -->
<div class="seccion">
  <h2>🔧 PROBAR SERVO</h2>
  <div class="panel">
    <div class="campo">
      <label>CANAL (4-10)</label>
      <select id="servoCh">
        <option value="4">4 - Párp.Sup</option>
        <option value="5">5 - Párp.Inf</option>
        <option value="6">6 - OjoIzqX</option>
        <option value="7">7 - OjoDerX</option>
        <option value="8">8 - OjoIzqY</option>
        <option value="9">9 - OjoDerY</option>
        <option value="10">10 - Boca</option>
      </select>
    </div>
    <div class="campo">
      <label>ÁNGULO (0-180)</label>
      <input type="number" id="servoAng" min="0" max="180" value="90">
    </div>
    <button class="btn-sm" onclick="probarServo()">ENVIAR</button>
  </div>
</div>

<!-- SECCIÓN: ALARMA -->
<div class="seccion">
  <h2>⏰ ALARMA FIESTA</h2>
  <div class="panel">
    <div class="campo">
      <label>HORA (0-23)</label>
      <input type="number" id="alH" min="0" max="23" value="7">
    </div>
    <div class="campo">
      <label>MINUTO (0-59)</label>
      <input type="number" id="alM" min="0" max="59" value="0">
    </div>
    <button class="btn-sm" onclick="setAlarma()">ACTIVAR</button>
    <button class="btn-sm rojo" onclick="clearAlarma()">APAGAR</button>
    <div class="alarma-estado" id="alEstado">OFF</div>
  </div>
</div>

<div class="toast" id="toast">OK</div>
<footer>MR. CUPCAKE BOT // 2026-I // TECNOLOGIA DIGITAL</footer>

<script>
function cmd(accion){
  fetch('/cmd?a='+accion).then(r=>r.text()).then(t=>mostrar(t)).catch(()=>mostrar('ERROR'));
}
function probarServo(){
  var ch=document.getElementById('servoCh').value;
  var ang=document.getElementById('servoAng').value;
  fetch('/cmd?a=servo&ch='+ch+'&ang='+ang).then(r=>r.text()).then(t=>mostrar(t)).catch(()=>mostrar('ERROR'));
}
function setAlarma(){
  var h=document.getElementById('alH').value;
  var m=document.getElementById('alM').value;
  fetch('/cmd?a=setalarma&h='+h+'&m='+m).then(r=>r.text()).then(t=>mostrar(t)).catch(()=>mostrar('ERROR'));
}
function clearAlarma(){
  fetch('/cmd?a=clearalarma').then(r=>r.text()).then(t=>mostrar(t)).catch(()=>mostrar('ERROR'));
}
function mostrar(msg){
  var t=document.getElementById('toast');
  t.textContent=msg; t.classList.add('show');
  setTimeout(()=>t.classList.remove('show'),1800);
}
function actualizarEstado(){
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('temp').textContent=d.temp!=-99?d.temp.toFixed(1)+'°C':'--°C';
    document.getElementById('hum').textContent=d.hum!=-99?d.hum.toFixed(0)+' %':'-- %';
    document.getElementById('cdInv').textContent=d.inv;
    document.getElementById('cdTemp').textContent=d.tmp;
    document.getElementById('alEstado').textContent=d.alarma||'OFF';
  });
}
actualizarEstado();
setInterval(actualizarEstado,5000);
</script></body></html>
)rawliteral";

// ─── Handlers del servidor ────────────────────────────────────────────────────
void handleRoot()   { server.send_P(200,"text/html",HTML_PAGE); }

void handleCmd() {
    if (!server.hasArg("a")) { server.send(400,"text/plain","?"); return; }
    String a = server.arg("a");

    // [CAMBIO 1] Prueba de servo desde la interfaz
    if (a == "servo") {
        if (server.hasArg("ch") && server.hasArg("ang")) {
            int ch  = server.arg("ch").toInt();
            int ang = server.arg("ang").toInt();
            if (ch >= 4 && ch <= 10 && ang >= 0 && ang <= 180) {
                server.send(200,"text/plain","OK: servo " + String(ch) + " -> " + String(ang) + "\xC2\xB0");
                moverServo(ch, ang, VEL_MEDIA);
                return;
            }
        }
        server.send(400,"text/plain","Error: canal 4-10, angulo 0-180");
        return;
    }

    // [CAMBIO 6] Configurar alarma
    if (a == "setalarma") {
        if (server.hasArg("h") && server.hasArg("m")) {
            alarmaHora = server.arg("h").toInt();
            alarmaMin  = server.arg("m").toInt();
            alarmaActiva = true;
            char buf[20];
            snprintf(buf, sizeof(buf), "Alarma: %02d:%02d", alarmaHora, alarmaMin);
            server.send(200,"text/plain", buf);
            return;
        }
        server.send(400,"text/plain","Error: falta hora/minuto");
        return;
    }

    if (a == "clearalarma") {
        alarmaActiva = false;
        alarmaHora = -1;
        alarmaMin  = -1;
        server.send(200,"text/plain","Alarma desactivada");
        return;
    }

    server.send(200,"text/plain","OK: "+a);

    // Ejecutar la acción correspondiente
    if      (a=="cerrar")      accionCerrar();
    else if (a=="sorpresa")    accionSorpresa();
    else if (a=="pestanear")   accionPestanear();
    else if (a=="sospechar")   accionSospechar();
    else if (a=="lados")       accionLados();
    else if (a=="ojoslocos")   accionOjosLocos();
    else if (a=="saludar")     accionSaludar();
    else if (a=="despedir")    accionDespedir();
    else if (a=="temperatura") funcTemperatura();
    else if (a=="investigar")  funcInvestigar();
    else if (a=="fiesta")      funcFiesta();
}

void handleStatus() {
    // Lectura rápida de sensores (no bloquea)
    float t2 = dht.readTemperature();
    float h2 = dht.readHumidity();
    if (!isnan(t2)) ultimaTemp = t2;
    if (!isnan(h2)) ultimaHum  = h2;

    String json = "{\"temp\":";
    json += String(ultimaTemp,1);
    json += ",\"hum\":";
    json += String(ultimaHum,0);
    json += ",\"inv\":\"";
    json += formatearTiempo(segParaInvestigar());
    json += "\",\"tmp\":\"";
    json += formatearTiempo(segParaTemperatura());
    json += "\",\"alarma\":\"";
    if (alarmaActiva) {
        char abuf[6];
        snprintf(abuf, sizeof(abuf), "%02d:%02d", alarmaHora, alarmaMin);
        json += String(abuf);
    } else {
        json += "OFF";
    }
    json += "\"}";
    server.send(200,"application/json",json);
}

// ═══════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);

    // ── Pin LED indicador
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    // ── Pines HC-SR04
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    // ── PCA9685
    pca.begin();
    pca.setOscillatorFrequency(27000000);
    pca.setPWMFreq(50);
    for (int i = 0; i < 16; i++) posActual[i] = 90;
    delay(100);
    // Centrar servos uno a uno
    for (int c = 4; c <= 10; c++) { _aplicar(c, 90); delay(80); }
    // Posición inicial correcta de boca
    _aplicar(BOCA, POS_BOCA_CERRADA);
    parpadoNormal();

    // ── NeoPixel
    strip.begin();
    strip.setBrightness(80);
    ledApagar();

    // ── DHT22
    dht.begin();

    // ── DFPlayer Mini (UART2, pines 16/17)
    dfSerial.begin(9600, SERIAL_8N1, 16, 17);
    if (player.begin(dfSerial)) {
        player.volume(20);
        Serial.println("DFPlayer OK");
    } else {
        Serial.println("DFPlayer no encontrado");
    }

    // ── WiFi Access Point
    WiFi.softAP(SSID_AP, PASS_AP);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    // ── NTP (funcionará si hay conexión a internet)
    configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);

    // ── Servidor web
    server.on("/",       handleRoot);
    server.on("/cmd",    handleCmd);
    server.on("/status", handleStatus);
    server.begin();
    Serial.println("Servidor HTTP listo");

    // ── Temporizadores automáticos (escalonados 30 min)
    tInvestigar  = millis() + INTERVALO_AUTO;
    tTemperatura = millis() + INTERVALO_AUTO + 1800000UL; // 30 min después

    // ── Arranque visual
    ledColor(strip.Color(155, 48, 255));  // Morado de arranque
    delay(400);
    ledApagar();
    Serial.println("Mr. Cupcake listo!");
}

// ═══════════════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════════════

// Verifica si la hora actual está entre 9 y 18 (9 am – 6 pm)
bool horarioActivo() {
    struct tm t;
    if (!getLocalTime(&t)) return true;  // Sin NTP → siempre activo
    return (t.tm_hour >= 9 && t.tm_hour < 18);
}

void loop() {
    server.handleClient();

    unsigned long ahora = millis();

    // ── Auto-investigar
    if (!funcEnCurso && ahora >= tInvestigar) {
        tInvestigar = ahora + INTERVALO_AUTO;
        if (horarioActivo()) funcInvestigar();
    }

    // ── Auto-temperatura
    if (!funcEnCurso && ahora >= tTemperatura) {
        tTemperatura = ahora + INTERVALO_AUTO;
        if (horarioActivo()) funcTemperatura();
    }

    // ── [CAMBIO 6] Verificar alarma
    if (alarmaActiva && !funcEnCurso) {
        struct tm t;
        if (getLocalTime(&t)) {
            if (t.tm_hour == alarmaHora && t.tm_min == alarmaMin) {
                alarmaActiva = false;  // Se dispara una sola vez
                Serial.println("ALARMA! Ejecutando fiesta...");
                funcAlarmaFiesta();
            }
        }
    }

    delay(10);  // Yield para el stack WiFi
}
