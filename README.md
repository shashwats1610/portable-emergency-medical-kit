# Portable Emergency Medical Kit

Embedded medical monitoring system: Arduino collects ECG, pulse oximetry, and IR temperature; displays on ILI9341 TFT; streams to Raspberry Pi over UART; Pi publishes JSON to Mosquitto over TLS. Optional SIM800L GSM HTTP backup.

## Architecture

```
Sensors (ECG, MAX30100, MLX90614) → Arduino → UART → Raspberry Pi → MQTT/TLS
                                    ↓
                              ILI9341 TFT
                                    ↓ (optional)
                              SIM800L → HTTP POST
```

## Project Structure

| Path | Description |
|------|-------------|
| [arduino/](arduino/) | Modular firmware (sensor, display, GSM managers) |
| [arduino/tests/](arduino/tests/) | Phase-1 per-sensor test sketches |
| [raspberry_pi/](raspberry_pi/) | UART gateway + MQTT client |
| [mosquitto/](mosquitto/) | Broker TLS configuration |
| [docs/WIRING.md](docs/WIRING.md) | Pin tables, power, voltage dividers |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Per-component debug guide |

## Quick Start

### Arduino

1. Install libraries (see [arduino/README.md](arduino/README.md)).
2. Wire per [docs/WIRING.md](docs/WIRING.md).
3. Upload `arduino/main.ino`.
4. Serial Monitor @ 9600 for telemetry: `HR:75,SPO2:98,TEMP:36.5,ECG:512`

### Raspberry Pi

1. `pip install -r raspberry_pi/requirements.txt`
2. `sudo bash raspberry_pi/scripts/setup_uart.sh` → reboot
3. `sudo bash raspberry_pi/scripts/setup_mosquitto.sh`
4. Update `raspberry_pi/config.yaml` password
5. `python3 raspberry_pi/gateway.py`

## Documentation

- [Wiring](docs/WIRING.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [Arduino README](arduino/README.md)
- [Pi Gateway README](raspberry_pi/README.md)
