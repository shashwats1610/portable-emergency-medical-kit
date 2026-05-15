#ifndef GSM_MANAGER_H
#define GSM_MANAGER_H

#include "config.h"
#include "sensor_manager.h"

void gsmManagerInit();
void gsmManagerUpdate();
void gsmManagerQueueVitals(const Vitals& vitals);
bool gsmManagerIsReady();

#endif // GSM_MANAGER_H
