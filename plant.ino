/*
 * ESP32 Plant Watering System with Servo and Ultrasonic Sensor (Test Mode)
 * 
 * Connections:
 * Water Level Sensor:
 *   VCC -> ESP32 3V3
 *   GND -> ESP32 GND
 *   Signal -> ESP32 GPIO 34
 * 
 * Ultrasonic Sensor (HC-SR04):
 *   VCC -> ESP32 5V
 *   GND -> ESP32 GND
 *   Trig -> ESP32 GPIO 5
 *   Echo -> ESP32 GPIO 18
 * 
 * Servo Motor:
 *   VCC -> External 5V power supply
 *   GND -> ESP32 GND + External power supply GND (common ground)
 *   Signal -> ESP32 GPIO 13
 * 
 * LCD1602 Module (Parallel):
 *   VSS -> ESP32 GND
 *   VDD -> ESP32 5V (or 3V3)
 *   V0 -> 10K potentiometer (for contrast adjustment)
 *   RS -> ESP32 GPIO 19
 *   RW -> ESP32 GND
 *   E -> ESP32 GPIO 23
 *   D4 -> ESP32 GPIO 25
 *   D5 -> ESP32 GPIO 27
 *   D6 -> ESP32 GPIO 26
 *   D7 -> ESP32 GPIO 15
 *   A (backlight +) -> ESP32 5V
 *   K (backlight -) -> ESP32 GND
 * 
 * LED:
 *   GPIO 2 -> 220Ω resistor -> LED (+) -> GND
 */

#include <LiquidCrystal.h>
#include <ESP32Servo.h>

// Pin Definitions
const int WATER_LEVEL_PIN = 34;
const int SOIL_MOISTURE_PIN = 35;
const int SERVO_PIN = 13;
const int LED_PIN = 2;
const int OVERRIDE_BUTTON= 32;

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(19, 23, 25, 27, 26, 15);

// Servo
Servo wateringServo;

// Thresholds
const int LOW_WATER_THRESHOLD = 500;
const int DRY_SOIL_THRESHOLD = 2500;  // Adjust after testing your sensor

// Servo positions (adjust these to your setup)
const int SERVO_UP_POSITION = 0;      // Cup upright (not watering)
const int SERVO_DOWN_POSITION = 90;   // Cup tipped (watering)

// SERVO SPEED CONTROL 
const int SERVO_SPEED_DELAY = 20;

// Timing variables
unsigned long lastWateringTime = 0;
const unsigned long WATERING_COOLDOWN = 10000;  // 10 seconds for testing (change to 3600000 for 1 hour)
const unsigned long WATERING_DURATION = 3000;    // 3 seconds pour time

// State variables
int waterLevelValue = 0;
int soilMoistureValue = 0;
bool isSoilDry = false;
bool isWaterLow = false;
bool isWatering = false;
bool isOverrideMode = false; 

unsigned long lastButtonPress = 0 ; 
const unsigned long  BUTTON_DEBOUNCE = 500 ; 
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Pin setup
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(OVERRIDE_BUTTON, INPUT_PULLUP);
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
  //lcd.print("TEST MODE!");
  
  // Calibration info
Serial.println("CALIBRATION MODE:");
Serial.println("Monitor soil moisture values for 30 seconds...\n");

// Read soil for calibration
for (int i = 0; i < 30; i++) {
  int reading = analogRead(SOIL_MOISTURE_PIN);
  Serial.print("Soil Reading: ");
  Serial.println(reading);
  delay(1000);
}

Serial.println("\nCalibration complete!");
Serial.println("Adjust DRY_SOIL_THRESHOLD in code if needed.\n");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read water level sensor
  waterLevelValue = analogRead(WATER_LEVEL_PIN);
  isWaterLow = (waterLevelValue < LOW_WATER_THRESHOLD);
  
 soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);

  // Check sensor states
  isSoilDry = (soilMoistureValue > DRY_SOIL_THRESHOLD);
  // Update LED
  digitalWrite(LED_PIN, isWaterLow ? HIGH : LOW);

  checkOverrideButton();
  
  // Automatic watering logic
  if (!isOverrideMode) {
  unsigned long currentTime = millis();
  
  if (!isWatering) {
    // Check if we should water
    if (isSoilDry && !isWaterLow) {
      // Check cooldown period
      if (currentTime - lastWateringTime > WATERING_COOLDOWN) {
        startWatering(true);
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
}

  
  // Serial output
  printStatus();
  
  delay(500);  // Faster updates for testing
}

void checkOverrideButton() {
  // Read button (LOW = pressed because of pull-up resistor)
  bool buttonPressed = (digitalRead(OVERRIDE_BUTTON) == LOW);
  
  if (buttonPressed) {
    unsigned long currentTime = millis();
    
    // Debounce: only trigger if enough time has passed since last press
    if (currentTime - lastButtonPress > BUTTON_DEBOUNCE) {
      lastButtonPress = currentTime;
      
      // Don't trigger if already watering
      if (!isWatering) {
        overrideWaterTrigger();
      } else {
        Serial.println(">>> Button pressed but already watering! <<<");
      }
    }
  }
}


void overrideWaterTrigger() {
  
  Serial.println("  OVERRIDE BUTTON PRESSED!        ");
  
  
  // Check if water is available
  if (isWaterLow) {
    Serial.println(">>> ERROR: Cannot water - reservoir is LOW! <<<");
    Serial.println(">>> Please refill water cup first! <<<\n");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("override: BLOCKED");
    lcd.setCursor(0, 1);
    lcd.print("Water cup empty!");
    delay(2000);
    return;
  }
  
  // Water is available - proceed with override watering!
  Serial.println(">>> override OVERRIDE ACTIVATED <<<");
  Serial.println(">>> Bypassing soil sensor and cooldown timer <<<");
  Serial.println(">>> Watering NOW! <<<\n");
  
  isOverrideMode = true;
  startWatering(true);  // true = override mode
}

void startWatering(bool overrideTrigger) {
  isWatering = true;
  isOverrideMode = overrideWaterTrigger;
  lastWateringTime = millis();
  
  Serial.println("\n>>> WATERING PLANT <<<");
  
  // Tip the cup down to pour water
  //wateringServo.write(SERVO_DOWN_POSITION);
  // Slowly tilt cup from 0 to 90
  for (int angle = SERVO_UP_POSITION; angle <= SERVO_DOWN_POSITION; angle++) {
    wateringServo.write(angle);
    delay(SERVO_SPEED_DELAY);  
  }
}

void stopWatering() {
  isWatering = false;
  
  // Return cup to upright position
  //wateringServo.write(SERVO_UP_POSITION);
   // Slowly return cup from 90 to 0
  for (int angle = SERVO_DOWN_POSITION; angle >= SERVO_UP_POSITION; angle--) {
    wateringServo.write(angle);
    delay(SERVO_SPEED_DELAY);  // Wait 20ms between each degree
  }
  
  
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
  }  else if (isSoilDry) {
  lcd.print("Soil is DRY!    ");
  lcd.setCursor(0, 1);
  lcd.print("Will water soon ");
} else {
  lcd.print("All systems OK  ");
  lcd.setCursor(0, 1);
  lcd.print("Soil: ");
  lcd.print(soilMoistureValue);
  lcd.print("    ");
}
  }


void printStatus() {
  Serial.print("Water Level: ");
  Serial.print(waterLevelValue);
  Serial.print(" / 4095");
  
 Serial.print("\t|\tSoil Moisture: ");
Serial.print(soilMoistureValue);
Serial.print(" / 4095 ");
Serial.print(isSoilDry ? "[DRY!]" : "[OK]");
  
  
  
  Serial.print("\t|\tReservoir: ");
  Serial.println(isWaterLow ? "LOW" : "OK");
}