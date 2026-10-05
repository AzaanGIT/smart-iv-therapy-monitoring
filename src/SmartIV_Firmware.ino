/*
 * Smart IV Monitoring System
 * Invention Disclosure Format (IDF-B) Prototype
 *
 * Microcontroller: Arduino Uno (ATmega328P)
 * Sensors:
 *   - HC-SR04 Ultrasonic Sensor (Monitors fluid level distance in IV bottle)
 *   - LDR + LED Optical Drop Counter (Detects drops falling in drip chamber)
 * Outputs:
 *   - 16x2 LCD Display (LiquidCrystal / I2C)
 *   - 5V Active Piezo Buzzer (Audio alarm when fluid < threshold)
 */

#include <Wire.h>
#include <LiquidCrystal.h>

// Pin definitions
const int TRIG_PIN = 9;         // HC-SR04 Ultrasonic Trigger
const int ECHO_PIN = 10;        // HC-SR04 Ultrasonic Echo
const int LDR_PIN  = A0;        // LDR Analog / Comparator Input
const int BUZZER_PIN = 8;       // 5V Active Buzzer Digital Pin

// Thresholds according to IDF specification
const float EMPTY_THRESHOLD_CM = 15.0; // Critical distance threshold (>15cm indicates low fluid)
const int LDR_DROP_THRESHOLD = 500;    // Optical threshold indicating drop occlusion

// LCD Pin configuration (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Runtime state variables
volatile unsigned long totalDrops = 0;
unsigned long lastDropMillis = 0;
float currentDistanceCm = 0.0;
bool isAlertActive = false;
unsigned long lastDisplayUpdate = 0;

// Measures fluid surface distance via HC-SR04 ultrasonic echo
float measureFluidDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m)
  if (duration == 0) return 999.0;

  // Sound speed = 340 m/s = 0.034 cm/us
  return (duration * 0.034) / 2.0;
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart IV Monitor");
  lcd.setCursor(0, 1);
  lcd.print("System Init...");
  delay(1500);
  lcd.clear();

  Serial.println(F("Smart IV Monitoring System Initialized"));
  Serial.println(F("IDF Document: IDF-B/2025/001"));
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Read LDR Drop Sensor
  int ldrValue = analogRead(LDR_PIN);
  if (ldrValue > LDR_DROP_THRESHOLD) {
    // Drop passing through LED-LDR optical line of sight
    if (currentMillis - lastDropMillis > 150) { // Software debounce (150ms)
      totalDrops++;
      lastDropMillis = currentMillis;
    }
  }

  // 2. Periodic Fluid Level Measurement & Display Refresh (Every 500ms)
  if (currentMillis - lastDisplayUpdate >= 500) {
    lastDisplayUpdate = currentMillis;
    currentDistanceCm = measureFluidDistance();

    // Check critical fluid level condition
    if (currentDistanceCm >= EMPTY_THRESHOLD_CM && currentDistanceCm < 300.0) {
      isAlertActive = true;
      digitalWrite(BUZZER_PIN, HIGH); // Sound audible warning
    } else {
      isAlertActive = false;
      digitalWrite(BUZZER_PIN, LOW);  // Silence alarm
    }

    // Refresh 16x2 LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Level: ");
    lcd.print(currentDistanceCm, 1);
    lcd.print("cm ");
    if (isAlertActive) {
      lcd.print("!LOW!");
    }

    lcd.setCursor(0, 1);
    lcd.print("Drops: ");
    lcd.print(totalDrops);

    // Telemetry output over serial
    Serial.print(F("Distance: "));
    Serial.print(currentDistanceCm);
    Serial.print(F(" cm | Drops: "));
    Serial.print(totalDrops);
    Serial.print(F(" | Alert: "));
    Serial.println(isAlertActive ? "CRITICAL" : "NORMAL");
  }
}
