const int PUMP_RELAY_PIN = 13;
const int OVERRIDE_BUTTON = 32;

const unsigned long WATERING_DURATION = 3000;
const unsigned long BUTTON_DEBOUNCE = 500;

bool isWatering = false;
unsigned long lastWateringTime = 0;
unsigned long lastButtonPress = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(OVERRIDE_BUTTON, INPUT_PULLUP);
  digitalWrite(PUMP_RELAY_PIN, LOW);
  Serial.println("Pump Test Ready - Press button to start");
}

void loop() {
  checkButton();
  
  if (isWatering) {
    if (millis() - lastWateringTime > WATERING_DURATION) {
      stopWatering();
    } else {
      unsigned long elapsed = millis() - lastWateringTime;
      Serial.print("Watering... ");
      Serial.print(elapsed / 1000.0, 1);
      Serial.println("s");
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
  Serial.println("Pump ON");
}

void stopWatering() {
  isWatering = false;
  digitalWrite(PUMP_RELAY_PIN, LOW);
  Serial.println("Pump OFF - Ready for next test");
}