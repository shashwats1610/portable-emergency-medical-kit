#include "display_manager.h"
#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// 3.3V only on TFT - 1k series on SPI if using 5V Uno

static Adafruit_ILI9341 tft = Adafruit_ILI9341(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

static const int ECG_GRAPH_X = 0;
static const int ECG_GRAPH_Y = 120;
static const int ECG_GRAPH_W = 320;
static const int ECG_GRAPH_H = 100;

static int prevHr = VITAL_INVALID;
static int prevSpO2 = VITAL_INVALID;
static float prevTemp = -999.0f;
static unsigned long lastVitalsDrawMs = 0;
static unsigned long lastEcgDrawMs = 0;
static bool labelsDrawn = false;

static void drawStaticLabels() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_LABEL, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print(F("Medical Kit"));
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print(F("HR:"));
  tft.setCursor(10, 60);
  tft.print(F("SpO2:"));
  tft.setCursor(160, 40);
  tft.print(F("Temp C:"));
  tft.drawRect(ECG_GRAPH_X, ECG_GRAPH_Y, ECG_GRAPH_W, ECG_GRAPH_H, COLOR_GRID);
  tft.setCursor(10, ECG_GRAPH_Y - 12);
  tft.print(F("ECG"));
  labelsDrawn = true;
}

static void drawStatusField(int x, const char* text, uint16_t color) {
  char pad[9];
  snprintf(pad, sizeof(pad), "%-8s", text);
  tft.setCursor(x, 95);
  tft.setTextColor(color, COLOR_BG);
  tft.print(pad);
}

static void drawSensorStatus() {
  tft.setTextSize(1);
  tft.setCursor(10, 95);
  tft.setTextColor(COLOR_FG, COLOR_BG);
  tft.print(F("Status:"));

  if (sensorIsPulseOxOk()) {
    drawStatusField(60, "POX", COLOR_ECG_TRACE);
  } else {
    drawStatusField(60, "POX ERR", COLOR_ERROR);
  }

  if (sensorIsTempOk()) {
    drawStatusField(110, "TEMP", COLOR_ECG_TRACE);
  } else {
    drawStatusField(110, "TEMP ERR", COLOR_ERROR);
  }

  if (isEcgLeadOn()) {
    drawStatusField(165, "ECG", COLOR_ECG_TRACE);
  } else {
    drawStatusField(165, "LEAD OFF", COLOR_ERROR);
  }
}

static void drawVitals(const Vitals& v) {
  char buf[16];

  tft.setTextSize(2);
  tft.setTextColor(COLOR_FG, COLOR_BG);

  tft.setCursor(50, 38);
  if (v.hrValid) {
    snprintf(buf, sizeof(buf), "%3d", v.hr);
  } else {
    snprintf(buf, sizeof(buf), " ---");
  }
  tft.print(buf);

  tft.setCursor(50, 58);
  if (v.spo2Valid) {
    snprintf(buf, sizeof(buf), "%3d", v.spo2);
    tft.print(buf);
    tft.print(F("%"));
  } else {
    tft.print(F(" ---"));
  }

  tft.setCursor(220, 38);
  if (v.tempValid) {
    snprintf(buf, sizeof(buf), "%5.1f", v.tempC);
  } else {
    snprintf(buf, sizeof(buf), "  ---");
  }
  tft.print(buf);
}

static void drawEcgWaveform() {
  // full redraw ~15ms - caps loop rate, pox.update() still runs every loop
  int count = getEcgBufferSize();
  if (count < 2 || !isEcgLeadOn()) {
    return;
  }

  tft.fillRect(ECG_GRAPH_X + 1, ECG_GRAPH_Y + 1, ECG_GRAPH_W - 2, ECG_GRAPH_H - 2, COLOR_BG);

  int samplesToDraw = min(count, ECG_GRAPH_W);
  int startIdx = count - samplesToDraw;

  int prevX = ECG_GRAPH_X;
  int prevY = ECG_GRAPH_Y + ECG_GRAPH_H / 2;

  for (int i = 0; i < samplesToDraw; i++) {
    int sample = getEcgBufferSample(startIdx + i);
    int x = ECG_GRAPH_X + i;
    int y = map(sample, 0, 1023, ECG_GRAPH_Y + ECG_GRAPH_H - 2, ECG_GRAPH_Y + 2);
    y = constrain(y, ECG_GRAPH_Y + 2, ECG_GRAPH_Y + ECG_GRAPH_H - 2);

    if (i > 0) {
      tft.drawLine(prevX, prevY, x, y, COLOR_ECG_TRACE);
    }
    prevX = x;
    prevY = y;
  }
}

void displayInit() {
  tft.begin();
  tft.setRotation(1);
  drawStaticLabels();
  drawSensorStatus();
  Serial.println(F("[DISPLAY] ILI9341 OK"));
}

void displayManagerUpdate() {
  if (!labelsDrawn) {
    return;
  }

  unsigned long now = millis();
  Vitals v = sensorManagerGetVitals();

  bool vitalsChanged = (v.hr != prevHr || v.spo2 != prevSpO2 ||
                        fabsf(v.tempC - prevTemp) > 0.05f);
  bool vitalsDue = (now - lastVitalsDrawMs >= (unsigned long)DISPLAY_REFRESH_MS);

  if (vitalsChanged || vitalsDue) {
    drawVitals(v);
    drawSensorStatus();
    prevHr = v.hr;
    prevSpO2 = v.spo2;
    prevTemp = v.tempC;
    lastVitalsDrawMs = now;
  }

  if (now - lastEcgDrawMs >= (unsigned long)ECG_DRAW_MS) {
    lastEcgDrawMs = now;
    drawEcgWaveform();
  }
}
