# Wiring Guide - Portable Emergency Medical Kit

## Pin Assignment Table

| Component | Signal | Arduino Pin | Notes |
|-----------|--------|---------------|-------|
| AD8232 ECG | OUTPUT | A0 | Analog input |
| AD8232 ECG | LO+ | 6 | Lead-off detect (HIGH = connected) |
| AD8232 ECG | LO- | 7 | Lead-off detect |
| ILI9341 TFT | CS | 10 | SPI chip select |
| ILI9341 TFT | DC | 8 | Data/command |
| ILI9341 TFT | RST | 9 | Reset |
| ILI9341 TFT | MOSI | 11 | Hardware SPI |
| ILI9341 TFT | MISO | 12 | Optional read |
| ILI9341 TFT | SCK | 13 | Hardware SPI |
| MAX30100 | SDA | A4 | I2C address 0x57 |
| MAX30100 | SCL | A5 | Shared I2C bus |
| MLX90614 | SDA | A4 | I2C address 0x5A |
| MLX90614 | SCL | A5 | Shared I2C bus |
| SIM800L | RX | Pin 3 | Arduino TX via voltage divider |
| SIM800L | TX | Pin 2 | Direct to Arduino RX |
| Pi Gateway | TX/RX | 1/0 | Hardware Serial @ 9600 baud |

**Pin conflict resolved:** ECG lead-off uses pins 6/7 instead of 10/11 to avoid SPI CS on pin 10.

---

## Power Rails

| Device | Voltage | Current | Supply |
|--------|---------|---------|--------|
| Arduino Uno/Nano | 5V | ~50mA | USB or 7-12V barrel |
| ILI9341 TFT | **3.3V only** | ~80mA | 3.3V pin (NOT 5V) |
| MAX30100 module | 3.3-5V | ~5mA | 3.3V preferred |
| MLX90614 | 3.3-5V | ~2mA | 3.3V or 5V |
| AD8232 | 3.3V internal | ~1mA | 3.3V or 5V module |
| SIM800L | **3.7-4.2V** | **up to 2A peak** | External buck/battery |

**CRITICAL:** Never connect ILI9341 VCC to 5V - permanent damage.

**CRITICAL:** Never power SIM800L from Arduino 5V/3.3V pins.

---

## Voltage Dividers

### Arduino TX → SIM800L RX (5V to 3.3V)

```
Arduino TX (5V) ----[10k]----+---- SIM800L RX
                             |
                           [20k]
                             |
                            GND

Output at SIM RX: 5V × 20k/(10k+20k) ≈ 3.33V
```

### Arduino TX → Raspberry Pi RX (5V to 3.3V)

Use the same divider topology on the Pi UART line:

```
Arduino TX (pin 1) ----[10k]----+---- Pi GPIO15 RX
                                |
                              [20k]
                                |
                               GND
```

Pi TX (GPIO14, 3.3V) → Arduino RX (pin 0) direct connection is safe.

---

## SPI Level Shifting (5V Arduino → 3.3V TFT)

If not using a 3.3V-level-shifter module, place **1kΩ series resistors** on:
- MOSI (pin 11)
- SCK (pin 13)
- CS (pin 10)
- DC (pin 8)

---

## MAX30100 RCWL-0530 Pull-Up Fix

**Symptom:** I2C scan finds no device at 0x57.

**Cause:** On many RCWL-0530 boards, SDA/SCL pull-ups connect to 1.8V instead of VCC.

**Fix:**
1. Remove three onboard 4.7kΩ pull-up resistors (if present to 1.8V rail).
2. Add external 4.7kΩ pull-ups from SDA and SCL to **3.3V**.

---

## ECG Electrode Placement (AD8232)

| Electrode | Body Location |
|-----------|---------------|
| RA | Right Arm |
| LA | Left Arm |
| RL | Right Leg (ground/driven) |

Check LO+ and LO- pins before trusting analog readings.

---

## SIM800L Wiring Checklist

- [ ] External 4.0V / 2A+ supply (LM2596 buck or 3.7V Li-Po)
- [ ] Common GND with Arduino
- [ ] GSM antenna attached **before** power-on
- [ ] 2G-enabled SIM card inserted
- [ ] TX voltage divider on module RX line
- [ ] SoftwareSerial on pins 2 (TX) / 3 (RX)

---

## Raspberry Pi UART

1. Disable serial console (`console=serial0,115200` removed from cmdline.txt).
2. `enable_uart=1` and `dtoverlay=disable-bt` in config.txt.
3. Use `/dev/serial0` (not `/dev/ttyS0` on Pi 3/4).
4. Voltage divider on Arduino TX → Pi RX.
