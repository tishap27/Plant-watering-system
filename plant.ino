/*
 * ESP32 Plant Watering System with Servo and Ultrasonic Sensor (Test Mode)
 * 
 * Connections:
 * Water Level Sensor:
 *   VCC → ESP32 3V3
 *   GND → ESP32 GND
 *   Signal → ESP32 GPIO 34
 * 
 * Ultrasonic Sensor (HC-SR04):
 *   VCC → ESP32 5V
 *   GND → ESP32 GND
 *   Trig → ESP32 GPIO 5
 *   Echo → ESP32 GPIO 18
 * 
 * Servo Motor:
 *   VCC → External 5V power supply
 *   GND → ESP32 GND + External power supply GND (common ground)
 *   Signal → ESP32 GPIO 13
 * 
 * LCD1602 Module (Parallel):
 *   VSS → ESP32 GND
 *   VDD → ESP32 5V (or 3V3)
 *   V0 → 10K potentiometer (for contrast adjustment)
 *   RS → ESP32 GPIO 19
 *   RW → ESP32 GND
 *   E → ESP32 GPIO 23
 *   D4 → ESP32 GPIO 25
 *   D5 → ESP32 GPIO 27
 *   D6 → ESP32 GPIO 26
 *   D7 → ESP32 GPIO 15
 *   A (backlight +) → ESP32 5V
 *   K (backlight -) → ESP32 GND
 * 
 * LED:
 *   GPIO 2 → 220Ω resistor → LED (+) → GND
 */

#include <LiquidCrystal.h>
#include <ESP32Servo.h>

// Pin Definitions
const int WATER_LEVEL_PIN = 34;
const int ULTRASONIC_TRIG_PIN = 5;
const int ULTRASONIC_ECHO_PIN = 18;
const int SERVO_PIN = 13;
const int LED_PIN = 2;

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(19, 23, 25, 27, 26, 15);

// Servo
Servo wateringServo;

// Thresholds
const int LOW_WATER_THRESHOLD = 500;
const int TRIGGER_DISTANCE_CM = 15;  // Trigger watering when object is closer than 15cm

// Servo positions (adjust these to your setup)
const int SERVO_UP_POSITION = 0;      // Cup upright (not watering)
const int SERVO_DOWN_POSITION = 90;   // Cup tipped (watering)

// Timing variables
unsigned long lastWateringTime = 0;
const unsigned long WATERING_COOLDOWN = 10000;  // 10 seconds for testing (change to 3600000 for 1 hour)
const unsigned long WATERING_DURATION = 3000;    // 3 seconds pour time

// State variables
int waterLevelValue = 0;
float distanceCm = 0;
bool isWaterLow = false;
bool shouldWater = false;
bool isWatering = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Pin setup
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Servo setup
  wateringServo.attach(SERVO_PIN);
  wateringServo.write(SERVO_UP_POSITION);  // Start with cup upright
  
  // LCD setup
  lcd.begin(16, 2);
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Plant Watering");
  lcd.setCursor(0, 1);
  lcd.print("TEST MODE!");
  
  Serial.println("\n=================================");
  Serial.println("ESP32 Plant Watering - TEST MODE");
  Serial.println("Using Ultrasonic Sensor");
  Serial.println("=================================\n");
  
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read water level sensor
  waterLevelValue = analogRead(WATER_LEVEL_PIN);
  isWaterLow = (waterLevelValue < LOW_WATER_THRESHOLD);
  
  // Read ultrasonic distance
  distanceCm = getDistance();
  
  // Check if object is close enough to trigger watering
  shouldWater = (distanceCm > 0 && distanceCm < TRIGGER_DISTANCE_CM);
  
  // Update LED
  digitalWrite(LED_PIN, isWaterLow ? HIGH : LOW);
  
  // Automatic watering logic
  unsigned long currentTime = millis();
  
  if (!isWatering) {
    // Check if we should water
    if (shouldWater && !isWaterLow) {
      // Check cooldown period
      if (currentTime - lastWateringTime > WATERING_COOLDOWN) {
        startWatering();
      } else {
        // Still in cooldown
        lcd.setCursor(0, 0);
        lcd.print("Cooldown active ");
        lcd.setCursor(0, 1);
        unsigned long timeLeft = (WATERING_COOLDOWN - (currentTime - lastWateringTime)) / 1000;
        lcd.print("Wait: ");
        lcd.print(timeLeft);
        lcd.print("s     ");
      }
    } else {
      // Update display
      updateDisplay();
    }
  } else {
    // Currently watering
    if (currentTime - lastWateringTime > WATERING_DURATION) {
      stopWatering();
    } else {
      lcd.setCursor(0, 0);
      lcd.print("WATERING...     ");
      lcd.setCursor(0, 1);
      unsigned long timeLeft = (WATERING_DURATION - (currentTime - lastWateringTime)) / 1000;
      lcd.print("Time: ");
      lcd.print(timeLeft);
      lcd.print("s       ");
    }
  }
  
  // Serial output
  printStatus();
  
  delay(500);  // Faster updates for testing
}

float getDistance() {
  // Send ultrasonic pulse
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  
  // Read echo pulse
  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000);  // 30ms timeout
  
  // Calculate distance in cm
  if (duration == 0) {
    return -1;  // No echo received
  }
  
  float distance = duration * 0.034 / 2;  // Speed of sound = 340 m/s
  return distance;
}

void startWatering() {
  isWatering = true;
  lastWateringTime = millis();
  
  Serial.println("\n>>> WATERING PLANT <<<");
  
  // Tip the cup down to pour water
  wateringServo.write(SERVO_DOWN_POSITION);
}

void stopWatering() {
  isWatering = false;
  
  // Return cup to upright position
  wateringServo.write(SERVO_UP_POSITION);
  
  Serial.println(">>> WATERING COMPLETE <<<");
  Serial.println(">>> SERVO RETURNED TO UP POSITION <<<\n");
  
  lcd.clear();
}

void updateDisplay() {
  lcd.setCursor(0, 0);
  
  if (isWaterLow) {
    lcd.print("REFILL WATER!   ");
    lcd.setCursor(0, 1);
    lcd.print("Tank is low     ");
  } else if (shouldWater) {
    lcd.print("Object detected!");
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    lcd.print((int)distanceCm);
    lcd.print("cm      ");
  } else {
    lcd.print("Ready to water  ");
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    if (distanceCm > 0) {
      lcd.print((int)distanceCm);
      lcd.print("cm      ");
    } else {
      lcd.print("---cm   ");
    }
  }
}

void printStatus() {
  Serial.print("Water Level: ");
  Serial.print(waterLevelValue);
  Serial.print(" / 4095");
  
  Serial.print("\t|\tDistance: ");
  if (distanceCm > 0) {
    Serial.print(distanceCm);
    Serial.print(" cm");
  } else {
    Serial.print("Out of range");
  }
  
  Serial.print("\t|\tTrigger: ");
  Serial.print(shouldWater ? "YES" : "NO");
  
  Serial.print("\t|\tReservoir: ");
  Serial.println(isWaterLow ? "LOW" : "OK");
}