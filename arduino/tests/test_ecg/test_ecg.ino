// Phase 1: AD8232 ECG test - Serial Monitor @ 115200
#define PIN_ECG_ANALOG    A0
#define PIN_ECG_LO_PLUS   6
#define PIN_ECG_LO_MINUS  7

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ECG_LO_PLUS, INPUT);
  pinMode(PIN_ECG_LO_MINUS, INPUT);
  Serial.println(F("AD8232 ECG Test - attach electrodes"));
}

void loop() {
  bool leadOn = digitalRead(PIN_ECG_LO_PLUS) == HIGH &&
                digitalRead(PIN_ECG_LO_MINUS) == HIGH;

  if (leadOn) {
    int value = analogRead(PIN_ECG_ANALOG);
    Serial.print(F("ECG:"));
    Serial.println(value);
  } else {
    Serial.println(F("LEAD OFF"));
  }
  delay(5);
}
