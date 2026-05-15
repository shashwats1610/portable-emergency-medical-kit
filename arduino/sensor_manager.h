#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "config.h"

struct Vitals {
  int hr;
  int spo2;
  int ecg;
  float tempC;
  bool hrValid;
  bool spo2Valid;
  bool tempValid;
  bool ecgValid;
};

void sensorManagerInit();
void sensorManagerUpdate();
Vitals sensorManagerGetVitals();

bool sensorIsEcgOk();
bool sensorIsPulseOxOk();
bool sensorIsTempOk();

bool isEcgLeadOn();
int getEcgBufferSize();
int getEcgBufferSample(int index);

void formatTelemetryLine(char* buf, size_t len);

#endif // SENSOR_MANAGER_H
