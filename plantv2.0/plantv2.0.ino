/*
 * ESP32 Simple Plant Watering System
 *
 * CONNECTIONS:
 * ─────────────────────────────────────
 * Water Level Sensor:
 *   VCC    -> ESP32 3.3V
 *   GND    -> ESP32 GND
 *   Signal -> ESP32 GPIO 34
 *
 * Soil Moisture Sensor:
 *   VCC    -> ESP32 3.3V
 *   GND    -> ESP32 GND
 *   Signal -> ESP32 GPIO 35
 *
 * Pump Relay Module:
 *   VCC    -> ESP32 5V
 *   GND    -> ESP32 GND
 *   IN     -> ESP32 GPIO 13
 *
 * LCD1602 (Parallel):
 *   VSS    -> ESP32 GND
 *   VDD    -> ESP32 5V
 *   V0     -> 10K potentiometer (contrast)
 *   RS     -> ESP32 GPIO 19
 *   RW     -> ESP32 GND
 *   E      -> ESP32 GPIO 23
 *   D4     -> ESP32 GPIO 25
 *   D5     -> ESP32 GPIO 27
 *   D6     -> ESP32 GPIO 26
 *   D7     -> ESP32 GPIO 15
 *   A      -> ESP32 5V  (backlight +)
 *   K      -> ESP32 GND (backlight -)
 *
 * LED:
 *   GPIO 2 -> 220Ω resistor -> LED (+) -> GND
 *
 * Buzzer:
 *   GPIO 4  -> Buzzer (+)
 *   GND     -> Buzzer (-)
 *
 * Override Button:
 *   GPIO 32 -> one leg
 *   GND     -> other leg  (uses internal pull-up)
 * ─────────────────────────────────────
 */

#include <LiquidCrystal.h>

// ── Pin Definitions ──────────────────
const int WATER_LEVEL_PIN  = 34;
const int SOIL_MOISTURE_PIN = 35;
const int PUMP_RELAY_PIN   = 5;
const int LED_PIN          = 2;
const int BUZZER_PIN       = 4;
const int OVERRIDE_BUTTON  = 32;

// ── LCD: RS, E, D4, D5, D6, D7 ──────
LiquidCrystal lcd(19, 23, 25, 27, 26, 14);

// ── Thresholds ───────────────────────
const int LOW_WATER_THRESHOLD = 500;   // below this = water low
const int DRY_SOIL_THRESHOLD  = 2500;  // above this = soil dry

// ── Timing ───────────────────────────
const unsigned long WATERING_COOLDOWN = 10000;  // 10s between auto-waterings
const unsigned long WATERING_DURATION = 3000;   // pump runs for 3s
const unsigned long BUTTON_DEBOUNCE   = 500;

// ── State Variables ───────────────────
int  waterLevelValue   = 0;
int  soilMoistureValue = 0;
bool isWaterLow  = false;
bool isSoilDry   = false;
bool isWatering  = false;
bool isOverride  = false;

unsigned long lastWateringTime = 0;
unsigned long lastButtonPress  = 0;

// ── Buzzer pattern state ──────────────
static unsigned long buzzerLastChange = 0;
static uint8_t       buzzerState      = 0;

// ─────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(WATER_LEVEL_PIN,   INPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(PUMP_RELAY_PIN,    OUTPUT);
  pinMode(LED_PIN,           OUTPUT);
  pinMode(BUZZER_PIN,        OUTPUT);
  pinMode(OVERRIDE_BUTTON,   INPUT_PULLUP);

  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(LED_PIN,        LOW);
  digitalWrite(BUZZER_PIN,     LOW);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Plant Watering");
  lcd.setCursor(0, 1);
  lcd.print("  System Ready");
  delay(2000);
  lcd.clear();
}

// ─────────────────────────────────────
void loop() {
  // 1. Read sensors
  waterLevelValue   = analogRead(WATER_LEVEL_PIN);
  soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);

  isWaterLow = (waterLevelValue   < LOW_WATER_THRESHOLD);
  isSoilDry  = (soilMoistureValue > DRY_SOIL_THRESHOLD);

  // 2. LED on when water is low
  digitalWrite(LED_PIN, isWaterLow ? HIGH : LOW);

  // 3. Buzzer triple-beep when water is low
  handleBuzzer();

  // 4. Check manual override button
  checkButton();

  // 5. Watering logic
  unsigned long now = millis();

  if (isWatering) {
    // ── Show watering progress ──
    unsigned long timeLeft = (WATERING_DURATION - (now - lastWateringTime)) / 1000;
    lcd.setCursor(0, 0);
    lcd.print(isOverride ? "MANUAL WATER    " : "AUTO  WATERING  ");
    lcd.setCursor(0, 1);
    lcd.print("Time left: ");
    lcd.print(timeLeft);
    lcd.print("s   ");

    // Stop when duration is up
    if (now - lastWateringTime >= WATERING_DURATION) {
      stopWatering();
    }

  } else {
    // ── Auto-start if soil is dry and water is available ──
    if (isSoilDry && !isWaterLow) {
      if (now - lastWateringTime >= WATERING_COOLDOWN) {
        startWatering(false);
      } else {
        unsigned long timeLeft = (WATERING_COOLDOWN - (now - lastWateringTime)) / 1000;
        lcd.setCursor(0, 0);
        lcd.print("Cooldown...     ");
        lcd.setCursor(0, 1);
        lcd.print("Wait: ");
        lcd.print(timeLeft);
        lcd.print("s       ");
      }
    } else {
      updateDisplay();
    }
  }

  printStatus();
  delay(300);
}

// ─────────────────────────────────────
void startWatering(bool override) {
  isWatering       = true;
  isOverride       = override;
  lastWateringTime = millis();
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  Serial.println(">>> PUMP ON <<<");
}

void stopWatering() {
  isWatering = false;
  digitalWrite(PUMP_RELAY_PIN, LOW);
  Serial.println(">>> PUMP OFF <<<");
  lcd.clear();
}

// ─────────────────────────────────────
void checkButton() {
  if (digitalRead(OVERRIDE_BUTTON) == LOW) {
    unsigned long now = millis();
    if (now - lastButtonPress > BUTTON_DEBOUNCE) {
      lastButtonPress = now;

      if (isWatering) {
        Serial.println("Button pressed - already watering!");
        return;
      }
      if (isWaterLow) {
        Serial.println("Button pressed - water too low!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BLOCKED!        ");
        lcd.setCursor(0, 1);
        lcd.print("Refill water!   ");
        delay(2000);
        lcd.clear();
        return;
      }
      Serial.println(">>> MANUAL OVERRIDE <<<");
      startWatering(true);
    }
  }
}

// ─────────────────────────────────────
void handleBuzzer() {
  const unsigned long shortBeep  = 80;
  const unsigned long shortGap   = 80;
  const unsigned long longPause  = 1000;
  unsigned long now = millis();

  if (!isWaterLow) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = 0;
    buzzerLastChange = now;
    return;
  }

  switch (buzzerState) {
    case 0: digitalWrite(BUZZER_PIN, HIGH);
            if (now - buzzerLastChange >= shortBeep)  { buzzerLastChange = now; buzzerState = 1; } break;
    case 1: digitalWrite(BUZZER_PIN, LOW);
            if (now - buzzerLastChange >= shortGap)   { buzzerLastChange = now; buzzerState = 2; } break;
    case 2: digitalWrite(BUZZER_PIN, HIGH);
            if (now - buzzerLastChange >= shortBeep)  { buzzerLastChange = now; buzzerState = 3; } break;
    case 3: digitalWrite(BUZZER_PIN, LOW);
            if (now - buzzerLastChange >= shortGap)   { buzzerLastChange = now; buzzerState = 4; } break;
    case 4: digitalWrite(BUZZER_PIN, HIGH);
            if (now - buzzerLastChange >= shortBeep)  { buzzerLastChange = now; buzzerState = 5; } break;
    case 5: digitalWrite(BUZZER_PIN, LOW);
            if (now - buzzerLastChange >= longPause)  { buzzerLastChange = now; buzzerState = 0; } break;
  }
}

// ─────────────────────────────────────
void updateDisplay() {
  lcd.setCursor(0, 0);
  if (isWaterLow) {
    lcd.print("REFILL WATER!   ");
    lcd.setCursor(0, 1);
    lcd.print("Tank is low     ");
  } else if (isSoilDry) {
    lcd.print("Soil: DRY!      ");
    lcd.setCursor(0, 1);
    lcd.print("Val: ");
    lcd.print(soilMoistureValue);
    lcd.print("       ");
  } else {
    lcd.print("Soil: OK        ");
    lcd.setCursor(0, 1);
    lcd.print("Val: ");
    lcd.print(soilMoistureValue);
    lcd.print("       ");
  }
}

// ─────────────────────────────────────
void printStatus() {
  Serial.print("Water Level: "); Serial.print(waterLevelValue);
  Serial.print(" | Soil: ");     Serial.print(soilMoistureValue);
  Serial.print(isSoilDry ? " [DRY]" : " [OK]");
  Serial.print(" | Reservoir: "); Serial.print(isWaterLow ? "LOW" : "OK");
  Serial.print(" | Pump: ");      Serial.println(isWatering ? "ON" : "OFF");
}