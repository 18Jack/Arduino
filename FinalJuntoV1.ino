#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <AccelStepper.h>

// ---------------- HARDWARE ----------------
#define DIR_PIN    3
#define STEP_PIN   4
#define MOTOR_TYPE 1   // usado por AccelStepper en tu ejemplo previo

#define OLED_ALTO       64
#define OLED_ANCHO      128
#define OLED_RESET_PIN  4

// ENCODER
const uint8_t PIN_A = 2;   // debe ser 2 o 3 en UNO para attachInterrupt()
const uint8_t PIN_B = 5;   // puede ser cualquier pin digital libre

// ---------------- CONFIGURACIÓN ----------------
// Encoder / distancia
const float DIAMETRO_MM = 42.0;  // mide con precisión (ó usa circunferencia/pi)
const int   PPR = 32;            // pulsos por vuelta por canal del encoder
const int   COUNTS_PER_PULSE = 2; // usamos CHANGE en A -> 2 counts por pulso "A"
const float MM_PER_COUNT = (PI * DIAMETRO_MM) / (PPR * COUNTS_PER_PULSE);

// Debounce (ajusta a 0 si encoder óptico)
const unsigned long DEBOUNCE_US = 1500; // 1000 µs = 1 ms (prueba 0 si hay pérdida)

// Intervalo de muestreo/actualización
const unsigned long PRINT_INTERVAL_MS = 1000;

// Stepper
AccelStepper stepper = AccelStepper(MOTOR_TYPE, STEP_PIN, DIR_PIN);
const float STEPPER_MAX_SPEED = 1000.0;
const float STEPPER_SPEED = 50.0;

// OLED
Adafruit_SSD1306 oled(OLED_ANCHO, OLED_ALTO, &Wire, OLED_RESET_PIN);

// ---------------- VARIABLES COMPARTIDAS ----------------
volatile long pulses = 0;                  // contador (puede ser + o -)
volatile unsigned long lastMicros = 0;     // para debounce dentro del ISR

unsigned long lastPrint = 0;
float distTotal_mm = 0.0;

// ---------------- ISR ----------------
// Se dispara cuando A cambia (CHANGE). Solo operaciones rápidas aquí.
void isrA() {
  unsigned long now = micros();
  if (DEBOUNCE_US > 0 && (now - lastMicros) < DEBOUNCE_US) return;
  lastMicros = now;

  uint8_t a = digitalRead(PIN_A);
  uint8_t b = digitalRead(PIN_B);

  // Regla: cuando A cambia, si a == b => avance, else => retroceso.
  // Si la dirección sale invertida, invierte ++/-- o la comparación.
  if (a == b) {
    pulses++;
  } else {
    pulses--;
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  Wire.begin();

  // OLED init
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("No se inicializó OLED");
    // no return; seguimos para que no bloquee la lógica principal
  }

  // Pines encoder
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);

  // Interrupción en A (CHANGE para mayor robustez)
  attachInterrupt(digitalPinToInterrupt(PIN_A), isrA, CHANGE);

  // Stepper
  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setSpeed(STEPPER_SPEED);

  // Inicial display
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
  oled.println("Iniciando...");
  oled.setCursor(0, 15);
  oled.setTextSize(2);
  oled.println("Integrament");
  oled.display();
  delay(2500);

  lastPrint = millis();
  Serial.println("Sistema listo.");
}

// ---------------- LOOP ----------------
void loop() {
  // Mantener el stepper corriendo (velocidad constante)
  stepper.runSpeed();

  unsigned long now = millis();
  if (now - lastPrint >= PRINT_INTERVAL_MS) {
    // Leer y resetear pulses de forma segura
    noInterrupts();
      long p = pulses;   // puede venir negativo
      pulses = 0;
    interrupts();

    // Solo sumamos al total los pulsos positivos (giro hacia adelante)
    long p_pos = (p > 0) ? p : 0;
    float distIntervalo_mm = p_pos * MM_PER_COUNT;
    distTotal_mm += distIntervalo_mm;

    // Serial debug
    Serial.print("pulses(raw): ");
    Serial.print(p);
    Serial.print("  pulses_added: ");
    Serial.print(p_pos);
    Serial.print("  dist_this_sec(mm): ");
    Serial.print(distIntervalo_mm, 3);
    Serial.print("  total(mm): ");
    Serial.println(distTotal_mm, 3);

    // OLED: limpiar y pintar
    oled.clearDisplay();
    // oled.drawRect(0, 0, OLED_ANCHO, OLED_ALTO, WHITE);

    oled.setTextSize(1);
    oled.setCursor(4, 4);
    oled.println("Distancia total:");

    oled.setTextSize(2);
    oled.setCursor(4, 14);
    oled.print(distTotal_mm, 1);
    oled.println(" mm"); // el println avanza línea, ok para texto pequeño

    // Mostrar último intervalo y raw pulses
    oled.setTextSize(1);
    oled.setCursor(4, 46);
    oled.print("Avance:");
    oled.print(distIntervalo_mm, 2);
    oled.setCursor(80, 46);
    oled.print("Raw:");
    oled.print(p);

    oled.display();

    lastPrint = now;
  }

  // aquí puedes añadir otras tareas sin bloquear
}
