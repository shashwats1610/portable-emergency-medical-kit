#ifndef CONFIG_H
#define CONFIG_H

// Pin and timing - see docs/WIRING.md
// HARDWARE WARNINGS (read before wiring):
// - ILI9341 TFT: 3.3V ONLY. 5V will permanently damage the display.
// - SIM800L: External 3.7-4.2V supply rated 2A+. Do NOT use Arduino 5V/3.3V.
// - SIM800L RX: Voltage divider required (Arduino TX 5V -> module RX 3.3V max).
//   Use 10k between Arduino TX and SIM RX, 20k from SIM RX to GND.
// - Pi gateway: Arduino TX (5V) -> voltage divider -> Pi GPIO15 RX (3.3V max).
// - MAX30100 RCWL-0530 modules may need I2C pull-up fix (see docs/WIRING.md).

// Plausible vitals ranges (filter garbage readings)
#define HR_MIN              40
#define HR_MAX              200
#define SPO2_MIN            70
#define SPO2_MAX            100

// --- Feature flags ---
#define ENABLE_UART_PI    1
#define ENABLE_GSM        0   // Set to 1 when SIM800L is wired and configured

// --- ECG (AD8232) ---
#define PIN_ECG_ANALOG      A0
#define PIN_ECG_LO_PLUS     6   // moved off 10 - fought SPI CS conflict for an hour
#define PIN_ECG_LO_MINUS    7

// --- ILI9341 TFT (hardware SPI) ---
#define PIN_TFT_CS          10
#define PIN_TFT_DC          8
#define PIN_TFT_RST         9
// MOSI=11, MISO=12, SCK=13 (hardware SPI on Uno/Nano)

// --- I2C (MAX30100 @ 0x57, MLX90614 @ 0x5A) ---
// SDA = A4, SCL = A5 (Wire library default)

// --- SIM800L (SoftwareSerial) ---
#define PIN_GSM_RX          3   // Connects to Arduino TX (via voltage divider)
#define PIN_GSM_TX          2   // Connects to SIM800L TX (3.3V safe for Arduino)
#define GSM_BAUD            9600

// --- UART to Raspberry Pi (hardware Serial pins 0/1) ---
#define SERIAL_PI_BAUD      9600

// --- Timing (non-blocking, millis-based) ---
#define ECG_SAMPLE_MS           5     // ~200 Hz
#define VITALS_PUBLISH_MS       500
#define DISPLAY_REFRESH_MS      500
#define TEMP_READ_MS            1000
#define TEMP_STABILIZE_MS       100   // Used only in setup() after MLX90614 begin
#define ECG_BUFFER_SIZE         200
#define ECG_DRAW_MS             33    // ~30 FPS max for TFT waveform

// --- Sensor I2C addresses ---
#define MAX30100_I2C_ADDR       0x57
#define MLX90614_I2C_ADDR       0x5A

// --- GSM / HTTP ---
#define GSM_APN                 "internet"   // Change for your carrier
#define GSM_SERVER_URL          "http://server.com/api/vitals"
#define GSM_CMD_TIMEOUT_MS      5000
#define GSM_REG_TIMEOUT_MS      30000
#define GSM_HTTP_TIMEOUT_MS     60000
#define GSM_POST_INTERVAL_MS    30000

// --- Display colors (16-bit RGB565) ---
#define COLOR_BG                0x0000
#define COLOR_FG                0xFFFF
#define COLOR_LABEL             0x07FF
#define COLOR_ERROR             0xF800
#define COLOR_ECG_TRACE         0x07E0
#define COLOR_GRID              0x3186

// --- Telemetry invalid sentinel ---
#define VITAL_INVALID           -1

#endif // CONFIG_H
