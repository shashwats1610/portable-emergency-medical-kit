#include "config.h"
#include "sensor_manager.h"
#include "display_manager.h"
#if ENABLE_GSM
#include "gsm_manager.h"
#endif

static char telemetryLine[80];
static unsigned long lastUartPublishMs = 0;

void setup() {
  Serial.begin(SERIAL_PI_BAUD);
  while (!Serial && millis() < 3000) {
    // Wait for USB serial on boards that need it (timeout 3s)
  }

  Serial.println(F("=== Portable Emergency Medical Kit ==="));

  sensorManagerInit();
  displayInit();

#if ENABLE_GSM
  gsmManagerInit();
#endif

  Serial.println(F("[MAIN] Setup complete"));
}

void loop() {
  sensorManagerUpdate();
  displayManagerUpdate();

#if ENABLE_UART_PI
  unsigned long now = millis();
  if (now - lastUartPublishMs >= (unsigned long)VITALS_PUBLISH_MS) {
    lastUartPublishMs = now;
    formatTelemetryLine(telemetryLine, sizeof(telemetryLine));
    Serial.println(telemetryLine);
  }
#endif

#if ENABLE_GSM
  gsmManagerUpdate();
  static unsigned long lastGsmQueueMs = 0;
  if (gsmManagerIsReady() && millis() - lastGsmQueueMs >= (unsigned long)GSM_POST_INTERVAL_MS) {
    lastGsmQueueMs = millis();
    gsmManagerQueueVitals(sensorManagerGetVitals());
  }
#endif
}
