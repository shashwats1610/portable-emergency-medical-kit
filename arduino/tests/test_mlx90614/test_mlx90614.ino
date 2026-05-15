// Phase 1: MLX90614 IR temperature test
#include <Wire.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!mlx.begin(0x5A)) {
    Serial.println(F("MLX90614 init FAILED"));
    while (1) {}
  }

  delay(100);
  Serial.println(F("MLX90614 OK - point at object"));
}

void loop() {
  Serial.print(F("Object C: "));
  Serial.print(mlx.readObjectTempC(), 2);
  Serial.print(F("  Ambient C: "));
  Serial.println(mlx.readAmbientTempC(), 2);
  delay(1000);
}
