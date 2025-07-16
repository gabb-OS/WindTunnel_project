// Lib per gestione Display LCD
#include <LiquidCrystal.h>

// Pin layout
const int pinLedRed = 8;
const int pinLedGreen = 9;
const int pinMosfetGate = 6;
const int pinKillSwitch = 13;
const int pinPotInput = A0;
const int pinBuzzer = A5;
LiquidCrystal lcd_1(12, 11, 5, 4, 3, 2);

// Dati fisici della ventola e della galleria del vento
const float fanCFM = 58.47;           // Portata in Cubic Feet per Minute
const float ductDiameter_cm = 7.3;    // Diametro interno tubo in cm

// Costanti di conversione
const float CFM_to_M3S = 0.00047194745; // 1 CFM = 0.00047194745 m³/s
//const float PI = 3.14159265;  // Definito in Arduino.h

// Costanti per la misurazione di RPM e KMH
const int maxRPM = 1200;
const int minRPM = 400;
float maxAirSpeed = 0.0;
int minEffectivePWM = 0;

// Threshold potenziometro
const int minPotThreshold = 50;   // Threshold iniziale potenziometro

int sensorValue = 0;
int outputValue = 0;

// Funzione per calcolo della velocità dell’aria (km/h)
float calculateMaxAirSpeed(float cfm, float diameter_cm) {
  float Q_m3s = cfm * CFM_to_M3S;                     // Portata Q: CFM to m3/s
  float diameter_m = diameter_cm / 100.0;             // diameter cm to m
  float area_m2 = PI * pow((diameter_m / 2.0), 2);    // Area A della sezione del tubo: in m2
  float speed_mps = Q_m3s / area_m2;                  // v = Q/A
  float speed_kmh = speed_mps * 3.6;                  // m/s to km/h
  return speed_kmh;
}

// Calcola il valore PWM teorico che corrisponde a minRPM di avvio della ventola (~400 RPM minimi)
// PWM = 400 * 255 / 1200 ≈ 85
int calculateMinEffectivePWM(int minRPM, int maxRPM) {
  return map(minRPM, 0, maxRPM, 0, 255);
}

// Funzione per aggiornare il display LCD
void updateLCD(int estimatedRPM, float estimatedKMH) {
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("RPM: ");
  lcd_1.print(estimatedRPM);
  lcd_1.setCursor(0, 1);
  lcd_1.print("KM/H: ");
  lcd_1.print(estimatedKMH, 2);
}

void setup() {
  //Serial.begin(9600);

  pinMode(pinLedRed, OUTPUT);
  pinMode(pinLedGreen, OUTPUT);
  pinMode(pinMosfetGate, OUTPUT);
  pinMode(pinKillSwitch, INPUT);
  pinMode(pinPotInput, INPUT);
  pinMode(pinBuzzer, OUTPUT);
  lcd_1.begin(16, 2);

  // Calcola la velocità massima dell’aria
  maxAirSpeed = calculateMaxAirSpeed(fanCFM, ductDiameter_cm);
  minEffectivePWM = calculateMinEffectivePWM(minRPM, maxRPM);
}

void loop() {
  // Controlla se è attivo il SAFETY STOP
  if (digitalRead(pinKillSwitch)) {
    lcd_1.clear();
    lcd_1.setCursor(0, 0);
    lcd_1.print("SAFETY STOP ON");
    lcd_1.setCursor(0, 1);
    lcd_1.print("Test aborted");

    // Segnalazione errore
    tone(pinBuzzer, 800);
    digitalWrite(pinLedGreen, LOW);
    digitalWrite(pinLedRed, HIGH);
    delay(300);
    tone(pinBuzzer, 300);
    digitalWrite(pinLedRed, LOW);
    digitalWrite(pinLedGreen, HIGH);
    delay(300);
    noTone(pinBuzzer);

    // Salta il resto del loop durante l'errore
    return; 
  }

  // Lettura potenziometro
  sensorValue = analogRead(pinPotInput);

  // Applica una soglia minima per evitare zone morte o instabilità meccanica del potenziometro
  sensorValue = analogRead(pinPotInput);
  outputValue = (sensorValue < minPotThreshold) ? 0 : map(sensorValue, minPotThreshold, 1023, minEffectivePWM, 255);
  analogWrite(pinMosfetGate, outputValue);

  int estimatedRPM = map(outputValue, 0, 255, 0, maxRPM);
  float estimatedKMH = maxAirSpeed * estimatedRPM / maxRPM;

  // Aggiorna il display LCD
  updateLCD(estimatedRPM, estimatedKMH);

  // Controllo LED
  bool isAboveThreshold = sensorValue >= minPotThreshold;
  digitalWrite(pinLedRed, isAboveThreshold ? HIGH : LOW);
  analogWrite(pinLedGreen, isAboveThreshold ? 0 : 75);

  // Debug serial output
  /*Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("  PWM = ");
  Serial.print(outputValue);
  Serial.print("  RPM = ");
  Serial.print(estimatedRPM);
  Serial.print("  KM/H = ");
  Serial.println(estimatedKMH, 1);*/

  //Non e' necessario un aggiornamento ultra rapido delle informazioni, ma rimane possibile farlo accorciando il delay
  delay(200);
}
