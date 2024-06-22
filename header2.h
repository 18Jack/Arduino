
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include "DHT.h"
#include <Servo.h>

#define DHTPIN 7     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define ENCODER_DT  8
#define ENCODER_CLK 2
#define SAVE_BUTTON 9
#define RELE_2 6 //ventilador
#define RELE_1 5 //foco
#define BUTTON_MENU 4
#define BUTTON_SERVO 3

Servo servo1;
// Inicializar la librería RTC
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// Variables para almacenar la hora de inicio
DateTime startTime;

float h, t, hic;

bool newContent = true; // Variable para indicar si hay nuevo contenido en la pantalla
bool selectMenu = true;
bool selectDays = true;
bool selectTemp = false;
bool selectVel = false;
bool selectAng = false;
bool selecttime = false;
bool selectconfig = false;
bool isMovingRight = true;          // Dirección del movimiento del servo
bool isServoRunning = false;        // Estado del servo

bool incubationComplete = false; 

bool isMoving = false;

int pos = 0;
int menuCount = 0; // Índice actual seleccionado por el usuario.
//int Countday = 15; //tiempo que ha transcurrido desde que inicio la incubación
int daysCount = 20;
int tempCount = 38;
int angCount = 180;// Máximo ángulo de movimiento del servo
int velCount = 15;// Velocidad de movimiento del servo en milisegundos por grado

int time = 1;

DateTime nextServoMove;
DateTime moveEndTime;

unsigned long lastExecutionTime = 0;

uint8_t arrow[8] = {
  0x00, 0x04, 0x06, 0x1f, 0x06, 0x04, 0x00
};

