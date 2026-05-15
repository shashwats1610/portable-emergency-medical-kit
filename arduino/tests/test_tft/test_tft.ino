// Phase 1: ILI9341 TFT test - 3.3V ONLY
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define PIN_TFT_CS   10
#define PIN_TFT_DC   8
#define PIN_TFT_RST  9

Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 20);
  tft.println(F("TFT OK"));

  tft.fillRect(20, 80, 60, 40, ILI9341_RED);
  tft.fillRect(90, 80, 60, 40, ILI9341_GREEN);
  tft.fillRect(160, 80, 60, 40, ILI9341_BLUE);
  tft.fillRect(230, 80, 60, 40, ILI9341_YELLOW);

  Serial.println(F("ILI9341 test pattern drawn"));
}

void loop() {}
