#include <Arduino.h>
constexpr const char *kBootBanner = "Advanced Timer V4 bootstrap (ESP32-S3)";

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(kBootBanner);
}

void loop() {
  delay(1000);
}
