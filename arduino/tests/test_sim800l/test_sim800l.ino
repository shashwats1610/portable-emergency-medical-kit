// Phase 1: SIM800L AT command test
// Wiring: External 4V/2A, antenna attached, TX divider on Arduino pin 3
#include <SoftwareSerial.h>

#define PIN_GSM_RX  3
#define PIN_GSM_TX  2

SoftwareSerial gsm(PIN_GSM_TX, PIN_GSM_RX);

void setup() {
  Serial.begin(115200);
  gsm.begin(9600);
  delay(2000);
  Serial.println(F("SIM800L Test - sending AT"));
  gsm.println(F("AT"));
}

void loop() {
  while (gsm.available()) {
    Serial.write(gsm.read());
  }
  while (Serial.available()) {
    gsm.write(Serial.read());
  }
}
