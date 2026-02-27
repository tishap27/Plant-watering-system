#include <TFT_eSPI.h>
#include "plant_image.h"   

TFT_eSPI tft = TFT_eSPI();

void showHappy() {
  tft.pushImage(0, 0, 240, 240, plant_pot_1_);

  // Text at bottom
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(30, 210);
  tft.print("I am happy! :)  ");
}

void showSad() {
  tft.pushImage(0, 0, 240, 240, plant_pot_1_);

  // Tint screen slightly blue to show sad
  tft.fillRect(0, 200, 240, 40, TFT_BLACK);

  // Text at bottom
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 210);
  tft.print("I need water! :(");
}

void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);  // Turn on backlight

  tft.init();
  tft.setRotation(2);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  showHappy();
  delay(3000);

  showSad();
  delay(3000);
}
