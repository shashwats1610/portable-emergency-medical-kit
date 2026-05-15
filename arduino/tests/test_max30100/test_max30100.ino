// Phase 1: MAX30100 pulse oximeter test
#include <Wire.h>
#include <MAX30100.h>
#include <MAX30100_BeatDetector.h>

MAX30100 pox;

void onBeatDetected() {}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!pox.begin()) {
    Serial.println(F("MAX30100 init FAILED - check I2C wiring/pull-ups"));
    while (1) {}
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  Serial.println(F("MAX30100 OK - place finger on sensor"));
}

void loop() {
  pox.update();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.print(F("HR:"));
    Serial.print(pox.getHeartRate());
    Serial.print(F(" SpO2:"));
    Serial.println(pox.getSpO2());
  }
}
