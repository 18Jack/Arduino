#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ---------------- OLED ----------------
#define OLED_ALTO       64
#define OLED_ANCHO      128
#define OLED_RESET_PIN  4
Adafruit_SSD1306 oled(OLED_ANCHO, OLED_ALTO, &Wire, OLED_RESET_PIN);

// ---------------- Pantallas ----------------
int pantallaActual = 0;     // Pantalla en la que estamos
int totalPantallas = 3;     // Número de pantallas que tendrás
bool botonPresionado = false;

// ---------------- ENCODER ----------------
const uint8_t PIN_A = 2;   // debe ser 2 o 3 en UNO para attachInterrupt()
const uint8_t PIN_B = 4;   // puede ser cualquier pin digital libre

// ---------------- NUEVOS PINES ----------------
const int pinSignalOut = 3; // Pin al transistor (colector → impresora)
const uint8_t PIN_BOTON  = 5; // botón toggle (INPUT_PULLUP)
const uint8_t PIN_BOTON2  = 6; // botón toggle (INPUT_PULLUP)

// ---------------- CONFIGURACIÓN ----------------
const float DIAMETRO_MM = 42.0;
const int   PPR = 32;
const int   COUNTS_PER_PULSE = 2;
const float MM_PER_COUNT = (PI * DIAMETRO_MM) / (PPR * COUNTS_PER_PULSE);

// Debounce ISR
const unsigned long DEBOUNCE_US = 1500;

// Intervalos
const unsigned long PRINT_INTERVAL_MS   = 1000;
const unsigned long tiempoMaxInactividad = 60000; // ms sin pulsos = runout
const unsigned long debounceBoton = 300;         // ms debounce botón

// ---------------- VARIABLES ----------------
volatile long pulses = 0;
volatile unsigned long lastMicros = 0;
unsigned long lastPrint = 0;
unsigned long ultimaActividad = 0;
float distTotal_mm = 0.0;
bool sensorHabilitado = false;
unsigned long lastButtonPress = 0; // para debounce

// ---------------- ISR ----------------
void isrA() {
  unsigned long now = micros();
  if (DEBOUNCE_US > 0 && (now - lastMicros) < DEBOUNCE_US) return;
  lastMicros = now;

  uint8_t a = digitalRead(PIN_A);
  uint8_t b = digitalRead(PIN_B);

  if (a == b) pulses++;
  else pulses--;

  ultimaActividad = millis(); // movimiento detectado
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Inicializar OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("No se inicializó OLED");
  }

  // Pines encoder
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);

  // Pines nuevos
  pinMode(pinSignalOut, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_BOTON2, INPUT_PULLUP);

  // Estado inicial -> filamento presente (LOW)
  //digitalWrite(pinSignalOut, LOW);

  // Interrupción en A
  attachInterrupt(digitalPinToInterrupt(PIN_A), isrA, CHANGE);

  // Pantalla inicial
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
  oled.println("Iniciando...");
  oled.setCursor(0, 15);
  oled.setTextSize(2);
  oled.println("Integramen");
  oled.display();

  delay(2000);
  lastPrint = millis();
  Serial.println("Sistema listo.");
}


// Función para mostrar contenido según la pantalla
void mostrarPantalla() {
  oled.clearDisplay();

  if (pantallaActual == 0) {
    oled.clearDisplay();
    oled.drawRect(0, 2, 126, 28, SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(6, 8);
    oled.print("Sensor ");
    oled.setTextSize(1);
    oled.setCursor(100, 12);
    oled.println(sensorHabilitado ? "ON" : "OFF");
    oled.drawRect(0, 32, 126, 28, SSD1306_WHITE);
    oled.setCursor(4, 38);
    oled.setTextSize(2);
    oled.print("Estado ");
    oled.setTextSize(1);
    oled.setCursor(85, 42);
    //oled.println((now - ultimaActividad < tiempoMaxInactividad) ? "MOVING" : "STOP");

  } else if (pantallaActual == 1) {
    oled.clearDisplay();
    oled.drawRect(0, 2, 126, 28, SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(6, 8);
    oled.print("Total ");
    oled.setTextSize(1);
    oled.setCursor(68, 12);
    oled.print(distTotal_mm, 1);
    oled.println(" mm");
    //oled.setCursor(4, 36);
    oled.drawRect(0, 32, 126, 28, SSD1306_WHITE);
    oled.setCursor(4, 38);
    oled.setTextSize(2);
    oled.print("Avance ");
    oled.setTextSize(1);
    //oled.print(distIntervalo_mm, 2);

  } else if (pantallaActual == 2) {
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print("Pantalla 3");
    oled.drawCircle(64, 32, 20, SSD1306_WHITE);
  }

  oled.display();
}



void loop() {
  // Lectura del botón (activo en LOW porque usamos pull-up)
  if (digitalRead(PIN_BOTON2) == LOW && !botonPresionado) {
    botonPresionado = true;
    pantallaActual++;
    if (pantallaActual >= totalPantallas) {
      pantallaActual = 0;  // Regresa a la primera pantalla
    }
    mostrarPantalla();
  }
  
  if (digitalRead(PIN_BOTON2) == HIGH) {
    botonPresionado = false; // Espera a soltar el botón
  }



  unsigned long now = millis();

  // -------- BOTON TOGGLE --------
  if (digitalRead(PIN_BOTON) == LOW && (now - lastButtonPress > debounceBoton)) {
    sensorHabilitado = !sensorHabilitado; // cambia estado
    ultimaActividad = millis();           // reinicia actividad si se activa
    lastButtonPress = now;
  }

// -------- SALIDA HACIA IMPRESORA --------
// LOW  = filamento ausente
// HIGH = filamento presente
if (!sensorHabilitado) {
  digitalWrite(pinSignalOut, HIGH);  // deshabilitado → siempre presente
} else {
  if (now - ultimaActividad < tiempoMaxInactividad) {
    digitalWrite(pinSignalOut, HIGH);  // movimiento → presente
  } else {
    digitalWrite(pinSignalOut, LOW);   // sin movimiento → runout
  }
}

  // -------- CÁLCULO DE DISTANCIA --------
  if (now - lastPrint >= PRINT_INTERVAL_MS) {
    noInterrupts();
    long p = pulses;
    pulses = 0;
    interrupts();

    long p_pos = (p > 0) ? p : 0;
    float distIntervalo_mm = p_pos * MM_PER_COUNT;
    distTotal_mm += distIntervalo_mm;

    Serial.print("pulses(raw): ");
    Serial.print(p);
    Serial.print("  dist_interval(mm): ");
    Serial.print(distIntervalo_mm, 3);
    Serial.print("  total(mm): ");
    Serial.println(distTotal_mm, 3);

    // -------- OLED --------

    // oled.setCursor(4, 48);
    // oled.print("Raw: ");
    // oled.print(p);
    //oled.display();

    lastPrint = now;
  }
}

