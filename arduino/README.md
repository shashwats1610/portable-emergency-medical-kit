# Arduino Firmware - Portable Emergency Medical Kit

## Required Libraries

Install via Arduino Library Manager or GitHub:

| Library | Source |
|---------|--------|
| MAX30100lib | OXullo Intersecans (NOT Adafruit MAX30100) |
| Adafruit MLX90614 | Adafruit |
| Adafruit GFX Library | Adafruit |
| Adafruit ILI9341 | Adafruit |
| Adafruit BusIO | Adafruit |

## Upload

1. Open `main.ino` in Arduino IDE (folder must be named `arduino` or copy files into a sketch folder).
2. Board: Arduino Uno or Nano.
3. Set `ENABLE_GSM` to `1` in `config.h` only when SIM800L is wired.
4. Upload; open Serial Monitor at **9600 baud** for debug and Pi telemetry.

## Telemetry Format (UART to Pi)

```
HR:75,SPO2:98,TEMP:36.5,ECG:512
```

Invalid readings use `-1` (e.g. `HR:-1` when finger not detected).

## Phase 1 Tests

Upload each sketch in `tests/` individually:

| Sketch | Validates |
|--------|-----------|
| `test_ecg` | AD8232 analog + lead-off pins 6/7 |
| `test_max30100` | I2C pulse oximeter |
| `test_mlx90614` | I2C temperature |
| `test_tft` | ILI9341 color bars |
| `test_sim800l` | SIM800L AT passthrough |

## Pin Summary

See [../docs/WIRING.md](../docs/WIRING.md) for full wiring tables and voltage divider diagrams.
