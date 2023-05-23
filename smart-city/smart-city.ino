// Library definition
#include <Wire.h>              //Library required for I2C comms (LCD)
#include <LiquidCrystal_I2C.h> //Library for LCD display via I2C
#include <math.h>              //Mathematics library for pow function (CO2 computation)

// I/O pin labeling
#define LDR1 0  // LDR Light sensor from traffic light 1 connected in pin A0
#define LDR2 1  // LDR Light sensor from traffic light 2 connected in pin A1
#define CO2 3   // CO2 sensor connected in pin A3
#define P1 37   // Traffic light 1 button connected in pin 37
#define P2 36   // Traffic light 2 button connected in pin 36
#define CNY1 35 // Infrared sensor 1 in traffic light 1 connected in pin 35
#define CNY2 34 // Infrared sensor 2 in traffic light 1 connected in pin 34
#define CNY3 33 // Infrared sensor 3 in traffic light 1 connected in pin 33
#define CNY4 32 // Infrared sensor 4 in traffic light 2 connected in pin 32
#define CNY5 31 // Infrared sensor 5 in traffic light 2 connected in pin 31
#define CNY6 30 // Infrared sensor 6 in traffic light 2 connected in pin 30
#define LR1 22  // Red traffic light 1 connected in pin 22
#define LY1 23  // Yellow traffic light 1 connected in pin 23
#define LG1 24  // Green traffic light 1 connected in pin 24
#define LR2 25  // Red traffic light 2 connected in pin 25
#define LG2 27  // Green traffic light 2 connected in pin 27
#define LY2 26  // Yellow traffic light 2 connected in pin 26

// Library definitions
LiquidCrystal_I2C lcd(0x27, 16, 4); //Set the LCD address to 0x27 for a 16 chars and 4 line display

/*---------------------------------------------MEF CO2-----------------------------------------------*/
#define OK 0
#define WARNING 1
// Constant definitions
//->CO2
const float DC_GAIN = 8.5; // define the DC gain of amplifier CO2 sensor
// const float ZERO_POINT_VOLTAGE = 0.4329; //define the output of the sensor in volts when the concentration of CO2 is 400PPM
const float ZERO_POINT_VOLTAGE = 0.265;                                                  // define the output of the sensor in volts when the concentration of CO2 is 400PPM
const float REACTION_VOLTAGE = 0.059;                                                    // define the “voltage drop” of the sensor when move the sensor from air into 1000ppm CO2
const float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTAGE / (2.602 - 3))}; // Line curve with 2 points

// Variable definitions
char comm = '\0'; // Command to test an actuator or sensor
float volts = 0;  // Variable to store current voltage from CO2 sensor
float co2 = 0;    // Variable to store CO2 value

unsigned long timerCo2 = 0;  // Variable to store time
unsigned char stateCo2 = OK; // Variable to store current CO2 MEF state
unsigned char maxCo2 = 100;  // Variable that indicate the max value of CO2 for warning

void MEF_CO2()
{
  // Realizar la lectura del voltaje del sensor de CO2
  if (millis() - timerCo2 >= 1000)
  {
    /*volts = analogRead(A0) * (5.0 / 1023.0); // Leer el valor analógico y convertirlo a voltaje (asumiendo una referencia de 5V)

      // Calcular la concentración de CO2 basada en el voltaje medido
      if (volts >= ZERO_POINT_VOLTAGE)
      {
      co2 = pow(10, (volts - CO2Curve[1]) / CO2Curve[0] + CO2Curve[2]); // Aplicar la fórmula de la curva de calibración
      }
      else
      {
      co2 = 0; // Si el voltaje es menor que el voltaje de calibración inicial, establecer la concentración de CO2 en 0
      }
      //  Serial.println("CO2: " + String(co2));*/
    co2 = analogRead(LDR1);
    timerCo2 = millis();
  }

  switch (stateCo2)
  {
    case OK:
      if (co2 <= maxCo2)
      {
        stateCo2 = WARNING;
        lcd.setCursor(0, 0);
        lcd.print("                   ");
        lcd.setCursor(0, 0);
        lcd.print("ALTO NIVEL DE CO2");
        Serial.println("WARNING");
      }
      break;
    case WARNING:
      if (co2 > maxCo2)
      {
        stateCo2 = OK;
        lcd.setCursor(0, 0);
        lcd.print("                   ");
        lcd.setCursor(0, 0);
        lcd.print("NIVEL CO2 ACEPTABLE");
        lcd.setCursor(0, 1);
        lcd.print("                   ");
        Serial.println("OK");
      }
      break;
  }
}


/*---------------------------------------------MEF Autos-----------------------------------------------*/
#define AUTOS_EN_TUNEL 1
#define TUNEL_VACIO 0

unsigned long timerAutos = 0;
unsigned char autos = TUNEL_VACIO;
unsigned long sensorAutos = 0;

void MEF_AUTOS()
{
  // Realizar la lectura del voltaje del sensor de CO2

  if (millis() - timerAutos >= 1000)
  {
    //sensorAutos = 0 Sin Autos, 1 = Con Autos
    sensorAutos = digitalRead(CNY4) == LOW || digitalRead(CNY5) == LOW || digitalRead(CNY6) == LOW;
    timerAutos = millis();
  }

  switch (autos)
  {
    case TUNEL_VACIO:
      if (sensorAutos)
      {
        autos = AUTOS_EN_TUNEL; // Cambiar al estado de luz verde
      }
      break;
    case AUTOS_EN_TUNEL:
      if (!sensorAutos)
      {
        autos = TUNEL_VACIO; // Cambiar al estado de luz verde
      }
      break;
  }
}

/*---------------------------------------------MEF SEMAFORO-----------------------------------------------*/
#define calleOn 1
#define tunelOn 0

// Estados del semáforo
enum State
{
  STATE_RED,    // Estado de luz roja
  STATE_YELLOW, // Estado de luz amarilla
  STATE_GREEN   // Estado de luz verde
};

// Variables de estado y temporizadores
State stateSC = STATE_RED;   // Estado inicial del semáforo Calle: rojo
State stateST = STATE_GREEN; // Estado inicial del semáforo Tunel: verde
unsigned long timerS = 0;    // Temporizador para cambiar el estado del semáforo Calle
unsigned long via = 1;       // via == 1 es paso para el tunnel, via==0 es paso para la calle

unsigned long timerYellow = 2000;
// Duración de los estados en milisegundos
const unsigned long YELLOW_DURATION = timerYellow; // Duración de la luz amarilla
const unsigned long GREEN_DURATION = 5000;         // Duración de la luz verde

// Controlar el semáforo 1
void MEF_SemaforizacionCalle()
{

  switch (stateSC)
  {
    case STATE_RED:
      // Encender luz roja y apagar las demás
      digitalWrite(LR1, HIGH);
      digitalWrite(LY1, LOW);
      digitalWrite(LG1, LOW);
      // Comprobar si es hora de cambiar de estado
      if (stateST == STATE_RED && (stateCo2 == OK || (stateCo2 == WARNING && autos == TUNEL_VACIO)))
      {
        stateSC = STATE_YELLOW; // Cambiar al estado de luz verde
        timerS = millis();      // Reiniciar el temporizador
        via = 1;                // la via la posee el tunnel
      }
      break;

    case STATE_YELLOW:
      // Encender luz amarilla y apagar las demás
      digitalWrite(LY1, HIGH);
      digitalWrite(LG1, LOW);
      digitalWrite(LR1, LOW);
      // Comprobar si es hora de cambiar de estado
      if (via == 1)
      {
        // el paso aun lo tiene el tunnel voy para GREEN
        timerYellow = 2000;
        if (millis() - timerS >= YELLOW_DURATION)
        {
          stateSC = STATE_GREEN; // Cambiar al estado de luz roja
          timerS = millis();
        }
      } else {
        // el paso lo tiene la calle voy para RED
        timerYellow = 1500;
        if (millis() - timerS >= YELLOW_DURATION)
        {
          stateSC = STATE_RED; // Cambiar al estado de luz roja
          timerS = millis();
        }
      }
      break;
    case STATE_GREEN:
      // Encender luz verde y apagar las demás
      digitalWrite(LG1, HIGH);
      digitalWrite(LR1, LOW);
      digitalWrite(LY1, LOW);
      // Comprobar si es hora de cambiar de estado
      if (millis() - timerS >= GREEN_DURATION && (stateCo2 == OK || (stateCo2 == WARNING && autos == AUTOS_EN_TUNEL)))
      {
        stateSC = STATE_YELLOW; // Cambiar al estado de luz amarilla
        timerS = millis();      // Reiniciar el temporizador
        via = 0;
      }
      break;
  }
};

// Controlar el semáforo 2
void MEF_SemaforizacionTunel()
{
  switch (stateST)
  {
    case STATE_RED:
      // Encender luz roja y apagar las demás
      digitalWrite(LR2, HIGH);
      digitalWrite(LY2, LOW);
      digitalWrite(LG2, LOW);
      // Comprobar si es hora de cambiar de estado
      if (stateSC == STATE_RED && (stateCo2 == OK || (stateCo2 == WARNING && autos == AUTOS_EN_TUNEL)))
      {
        stateST = STATE_YELLOW; // Cambiar al estado de luz verde
        timerS = millis();      // Reiniciar el temporizador
        via = 0;                // la via la posee la calle
      }
      break;
    case STATE_YELLOW:
      // Encender luz amarilla y apagar las demás
      digitalWrite(LY2, HIGH);
      digitalWrite(LR2, LOW);
      digitalWrite(LG2, LOW);
      // Comprobar si es hora de cambiar de estado
      if (via == 1)
      {
        // el paso aun lo tiene el tunnel voy para RED
        timerYellow = 1500;
        if (millis() - timerS >= YELLOW_DURATION)
        {
          stateST = STATE_RED; // Cambiar al estado de luz roja
          timerS = millis();
        }
      }
      else
      {
        // el paso lo tiene la calle voy para GREEN
        timerYellow = 2000;
        if (millis() - timerS >= YELLOW_DURATION)
        {
          stateST = STATE_GREEN; // Cambiar al estado de luz roja
          timerS = millis();
        }
      }
      break;
    case STATE_GREEN:
      // Encender luz verde y apagar las demás
      digitalWrite(LR2, LOW);
      digitalWrite(LY2, LOW);
      digitalWrite(LG2, HIGH);
      // Comprobar si es hora de cambiar de estado
      if (millis() - timerS >= GREEN_DURATION && (stateCo2 == OK || (stateCo2 == WARNING && autos == TUNEL_VACIO)) )
      {
        stateST = STATE_YELLOW; // Cambiar al estado de luz amarilla
        timerS = millis();      // Reiniciar el temporizador
        via = 1;
      }

      break;
  }
};

String readSerial() {
  if (Serial.available() > 0) { //Check if there are bytes sent from PC
    String palabra = "Temperatura: ";
    palabra += Serial.readStringUntil('/'); 
    lcd.setCursor(0, 1);
    lcd.print("                   ");
    lcd.setCursor(0, 1);
    lcd.print(palabra);
  }
}


void setup()
{
  // Pin config
  pinMode(P1, INPUT);   // Traffic light 1 button as Input
  pinMode(P2, INPUT);   // Traffic light 2 button as Input
  pinMode(CNY1, INPUT); // Infrared sensor 1 in traffic light 1 as Input
  pinMode(CNY2, INPUT); // Infrared sensor 2 in traffic light 1 as Input
  pinMode(CNY3, INPUT); // Infrared sensor 3 in traffic light 1 as Input
  pinMode(CNY4, INPUT); // Infrared sensor 4 in traffic light 2 as Input
  pinMode(CNY5, INPUT); // Infrared sensor 5 in traffic light 2 as Input
  pinMode(CNY6, INPUT); // Infrared sensor 6 in traffic light 2 as Input
  pinMode(LR1, OUTPUT); // Red traffic light 1 as Output
  pinMode(LY1, OUTPUT); // Yellow traffic light 1 as Output
  pinMode(LG1, OUTPUT); // Green traffic light 1 as Output
  pinMode(LR2, OUTPUT); // Red traffic light 2 as Output
  pinMode(LY2, OUTPUT); // Yellow traffic light 2 as Output
  pinMode(LG2, OUTPUT); // Green traffic light 2 as Output

  // Output cleaning
  digitalWrite(LR1, LOW); // Turn Off Red traffic light 1
  digitalWrite(LY1, LOW); // Turn Off Yellow traffic light 1
  digitalWrite(LG1, LOW); // Turn Off Green traffic light 1
  digitalWrite(LR2, LOW); // Turn Off Red traffic light 2
  digitalWrite(LY2, LOW); // Turn Off Yellow traffic light 2
  digitalWrite(LG2, LOW); // Turn Off Green traffic light 2

  // Communications
  Serial.begin(9600); //Start Serial communications with computer via Serial0 (TX0 RX0) at 9600 bauds
  lcd.begin();  //Start communications with LCD display
  lcd.backlight();  //Turn on LCD backlight
  lcd.clear();
  lcd.setCursor(0, 0);
  if (stateCo2 == OK) {
    lcd.print("NIVEL CO2 ACEPTABLE");
  } else {
    lcd.print("ALTO NIVEL DE CO2");
  }
}

void loop()
{
  MEF_SemaforizacionCalle();
  MEF_SemaforizacionTunel();
  MEF_CO2();
  MEF_AUTOS();
  readSerial();
}
