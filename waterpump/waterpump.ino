#include <LiquidCrystal.h>

const int PUMP_RELAY_PIN = 13;
const int OVERRIDE_BUTTON = 32;

LiquidCrystal lcd(19, 23, 25, 27, 26, 15);

const unsigned long WATERING_DURATION = 3000;
const unsigned long BUTTON_DEBOUNCE = 500;

bool isWatering = false;
unsigned long lastWateringTime = 0;
unsigned long lastButtonPress = 0;

void setup() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(OVERRIDE_BUTTON, INPUT_PULLUP);
  digitalWrite(PUMP_RELAY_PIN, LOW);
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pump Test");
  lcd.setCursor(0, 1);
  lcd.print("Press button");
}

void loop() {
  checkButton();
  
  if (isWatering) {
    if (millis() - lastWateringTime > WATERING_DURATION) {
      stopWatering();
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Watering");
      lcd.setCursor(0, 1);
      lcd.print("Time: ");
      
    }
  }
  
  delay(100);
}

void checkButton() {
  if (digitalRead(OVERRIDE_BUTTON) == LOW) {
    unsigned long now = millis();
    if (now - lastButtonPress > BUTTON_DEBOUNCE && !isWatering) {
      lastButtonPress = now;
      startWatering();
    }
  }
}

void startWatering() {
  isWatering = true;
  lastWateringTime = millis();
  digitalWrite(PUMP_RELAY_PIN, HIGH);
}

void stopWatering() {
  isWatering = false;
  digitalWrite(PUMP_RELAY_PIN, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pump Test");
  lcd.setCursor(0, 1);
  lcd.print("Press button");
}