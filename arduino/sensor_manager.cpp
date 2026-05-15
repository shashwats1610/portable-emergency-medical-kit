#include "sensor_manager.h"
#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <MAX30100.h>
#include <MAX30100_BeatDetector.h>
#include <Adafruit_MLX90614.h>

static MAX30100 pox;
static Adafruit_MLX90614 mlx = Adafruit_MLX90614();

static Vitals currentVitals;
static bool pulseOxOk = false;
static bool tempOk = false;
static bool ecgOk = false;

static int ecgBuffer[ECG_BUFFER_SIZE];
static int ecgWriteIndex = 0;
static int ecgCount = 0;
static int latestEcgRaw = 0;

static unsigned long lastEcgMs = 0;
static unsigned long lastTempMs = 0;

static void onBeatDetected() {
  // callback required by lib
}

static bool checkEcgLeadOn() {
  return digitalRead(PIN_ECG_LO_PLUS) == HIGH &&
         digitalRead(PIN_ECG_LO_MINUS) == HIGH;
}

static bool hrInRange(int hr) {
  return hr >= HR_MIN && hr <= HR_MAX;
}

static bool spo2InRange(int spo2) {
  return spo2 >= SPO2_MIN && spo2 <= SPO2_MAX;
}

void sensorManagerInit() {
  memset(&currentVitals, 0, sizeof(currentVitals));
  currentVitals.hr = VITAL_INVALID;
  currentVitals.spo2 = VITAL_INVALID;
  currentVitals.ecg = VITAL_INVALID;
  currentVitals.tempC = 0.0f;

  pinMode(PIN_ECG_LO_PLUS, INPUT);
  pinMode(PIN_ECG_LO_MINUS, INPUT);

  Wire.begin();

  // Spent ages on this - RCWL-0530 needs pullups to 3.3V not 1.8V rail
  if (!pox.begin()) {
    Serial.println(F("[SENSOR] MAX30100 init FAILED - check I2C pull-ups"));
    pulseOxOk = false;
  } else {
    pox.setOnBeatDetectedCallback(onBeatDetected);
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pulseOxOk = true;
    Serial.println(F("[SENSOR] MAX30100 OK"));
  }

  if (!mlx.begin(MLX90614_I2C_ADDR)) {
    Serial.println(F("[SENSOR] MLX90614 init FAILED"));
    tempOk = false;
  } else {
    delay(TEMP_STABILIZE_MS);
    tempOk = true;
    Serial.println(F("[SENSOR] MLX90614 OK"));
  }

  ecgOk = true;
  Serial.println(F("[SENSOR] AD8232 ECG ready (analog)"));
}

void sensorManagerUpdate() {
  unsigned long now = millis();

  if (pulseOxOk) {
    pox.update();
    int hr = pox.getHeartRate();
    int spo2 = pox.getSpO2();
    currentVitals.hr = hr;
    currentVitals.spo2 = spo2;
    currentVitals.hrValid = hrInRange(hr);
    currentVitals.spo2Valid = spo2InRange(spo2);
    //Serial.print(F("raw hr/spo2: ")); Serial.print(hr); Serial.println(spo2);
  }

  if (ecgOk && (now - lastEcgMs >= (unsigned long)ECG_SAMPLE_MS)) {
    lastEcgMs = now;
    bool leadOn = checkEcgLeadOn();
    currentVitals.ecgValid = leadOn;

    if (leadOn) {
      delayMicroseconds(100);  // ADC settle - datasheet says ~100us
      latestEcgRaw = analogRead(PIN_ECG_ANALOG);
      currentVitals.ecg = latestEcgRaw;
      ecgBuffer[ecgWriteIndex] = latestEcgRaw;
      ecgWriteIndex = (ecgWriteIndex + 1) % ECG_BUFFER_SIZE;
      if (ecgCount < ECG_BUFFER_SIZE) {
        ecgCount++;
      }
    } else {
      currentVitals.ecg = VITAL_INVALID;
    }
  }

  if (tempOk && (now - lastTempMs >= (unsigned long)TEMP_READ_MS)) {
    lastTempMs = now;
    delayMicroseconds(100);  // let I2C bus settle after pox.update()
    currentVitals.tempC = mlx.readObjectTempC();
    currentVitals.tempValid = !isnan(currentVitals.tempC);
  }
}

Vitals sensorManagerGetVitals() {
  return currentVitals;
}

bool sensorIsEcgOk() { return ecgOk; }
bool sensorIsPulseOxOk() { return pulseOxOk; }
bool sensorIsTempOk() { return tempOk; }

bool isEcgLeadOn() {
  return checkEcgLeadOn();
}

int getEcgBufferSize() {
  return ecgCount;
}

int getEcgBufferSample(int index) {
  if (index < 0 || index >= ecgCount) {
    return 0;
  }
  int start = (ecgWriteIndex - ecgCount + ECG_BUFFER_SIZE) % ECG_BUFFER_SIZE;
  int pos = (start + index) % ECG_BUFFER_SIZE;
  return ecgBuffer[pos];
}

void formatTelemetryLine(char* buf, size_t len) {
  if (len == 0) {
    return;
  }

  Vitals v = currentVitals;
  int offset = 0;

  offset += snprintf(buf + offset, len - offset, "HR:%d",
                     v.hrValid ? v.hr : VITAL_INVALID);
  offset += snprintf(buf + offset, len - offset, ",SPO2:%d",
                     v.spo2Valid ? v.spo2 : VITAL_INVALID);
  offset += snprintf(buf + offset, len - offset, ",TEMP:%.1f",
                     v.tempValid ? v.tempC : (float)VITAL_INVALID);
  offset += snprintf(buf + offset, len - offset, ",ECG:%d",
                     v.ecgValid ? v.ecg : VITAL_INVALID);

  buf[len - 1] = '\0';
}
