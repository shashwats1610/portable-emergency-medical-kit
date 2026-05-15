# Portable Emergency Medical Kit

Low-cost vitals monitor for the gap between an accident and professional care. In remote or disaster settings, ambulances can take 30+ minutes; baseline heart rhythm, SpO2, and temperature from the first minutes are often lost by the time paramedics arrive. This device captures those readings on scene, shows them on a built-in screen, and relays them to a remote listener over a secure MQTT link.

**Proof-of-concept / portfolio project** - not a certified medical device. It provides data for bystanders and clinicians; it does not diagnose.

## What it measures

| Sensor | Reading | Why it matters on scene |
|--------|---------|-------------------------|
| AD8232 | ECG waveform + lead-off detect | See if rhythm looks regular; spot gross changes before help arrives |
| MAX30100 | Heart rate, SpO2 | Hypoxia (SpO2 below 90%) and shock trends (HR rising or falling) |
| MLX90614 | Object temperature (IR, non-contact) | Check temp without touching burns, wounds, or suspected spine injury |
| ILI9341 TFT | Live display | Bystanders can read values and report them to dispatch |

Readings are filtered to plausible ranges (e.g. HR 40-200 BPM, SpO2 70-100%) before upload.

## How data leaves the device

**Default path (enabled in firmware):** Arduino → UART (9600 baud) → Raspberry Pi gateway → JSON on MQTT topic `medical_kit/vitals` (TLS port 8883, local Mosquitto).

**Alternate path (off by default):** Set `ENABLE_GSM` to `1` in [`arduino/config.h`](arduino/config.h) to send the same vitals via SIM800L **HTTP POST** when cellular hardware is wired. No WiFi or smartphone required for the Arduino side; the Pi path needs the gateway running where UART is connected.

```
Sensors → Arduino → UART → Raspberry Pi → MQTT/TLS (default)
              ↓
         ILI9341 TFT
              ↓  (ENABLE_GSM=1 only)
         SIM800L → HTTP POST
```

## What this is / is not

| This is | This is not |
|---------|-------------|
| A bridge for the first minutes before advanced care | A replacement for hospital monitors or paramedic gear |
| Real-time display + remote data relay | A diagnostic tool |
| Built from ~₹3,000-5,000 in parts (target BOM) | FDA/CE cleared equipment |

## Repository layout

| Path | Description |
|------|-------------|
| [arduino/](arduino/) | Firmware: `sensor_manager`, `display_manager`, `gsm_manager`, `main.ino` |
| [arduino/tests/](arduino/tests/) | Per-sensor bring-up sketches |
| [raspberry_pi/](raspberry_pi/) | Serial parser, MQTT client, `gateway.py`, setup scripts |
| [mosquitto/](mosquitto/) | TLS broker config template |
| [docs/WIRING.md](docs/WIRING.md) | Pins, power, voltage dividers |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Common hardware failures |
| [TESTING_NOTES.md](TESTING_NOTES.md) | Bench tests and expected ranges |
| [BUGS_FOUND.md](BUGS_FOUND.md) | Known issues fixed in this revision |

## Quick start

### Arduino

1. Install libraries - see [arduino/README.md](arduino/README.md).
2. Wire per [docs/WIRING.md](docs/WIRING.md) (TFT **3.3 V only**; SIM800L needs external 4 V / 2 A if used).
3. Upload [`arduino/main.ino`](arduino/main.ino).
4. Telemetry on Serial @ 9600: `HR:75,SPO2:98,TEMP:36.5,ECG:512`

### Raspberry Pi gateway

1. `pip install -r raspberry_pi/requirements.txt`
2. `cp raspberry_pi/config.yaml.example raspberry_pi/config.yaml` and set MQTT credentials (or `export MQTT_PASSWORD=...`)
3. `sudo bash raspberry_pi/scripts/setup_uart.sh` then reboot
4. `sudo bash raspberry_pi/scripts/setup_mosquitto.sh`
5. `python3 raspberry_pi/gateway.py`

Subscribe locally:

```bash
mosquitto_sub -h 127.0.0.1 -p 8883 --cafile /etc/mosquitto/certs/ca.crt \
  -u medical_gateway -P YOUR_PASSWORD -t medical_kit/vitals -v
```

## License

Add a license file if you publish this repo publicly.
