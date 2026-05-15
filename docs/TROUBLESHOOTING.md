# Troubleshooting Guide

## AD8232 ECG

| Symptom | Cause | Fix |
|---------|-------|-----|
| Flat line / noise | Electrodes off skin | Check LO+ and LO- both HIGH before reading A0 |
| Serial shows LEAD OFF | Poor electrode contact | Re-apply gel electrodes; clean skin |
| Erratic values | 50/60 Hz mains hum | Keep leads short; away from AC power |
| No waveform on TFT | Lead-off or wrong pins | Use pins 6/7 for LO+, LO- (not 10/11) |

---

## MAX30100 Pulse Oximeter

| Symptom | Cause | Fix |
|---------|-------|-----|
| Init FAILED on Serial | Wrong pull-ups (RCWL-0530) | Remove 1.8V pull-ups; add 4.7k to 3.3V |
| HR/SpO2 always 0 | Finger not placed | Cover LED/photodiode firmly |
| HR/SpO2 always 0 | LED current not set | Call `setIRLedCurrent()` in setup |
| Missed beats | `update()` not frequent | Call `pox.update()` every `loop()` - no `delay()` |
| Compile errors | Wrong library | Use **MAX30100lib** by OXullo, not Adafruit |
| I2C conflict | Multiple `Wire.begin()` | Only call `Wire.begin()` once in sensor_manager |

---

## MLX90614 Temperature

| Symptom | Cause | Fix |
|---------|-------|-----|
| Init FAILED | Wrong address or wiring | Verify 0x5A on I2C scan |
| NaN readings | Read too soon after begin | Wait 100ms after `begin()` in setup |
| Inaccurate object temp | Distance / FOV | Hold 2-10 cm; 90 deg cone - measure center spot |
| Reads ambient only | Aiming at wrong target | Point sensor window at skin/object |

---

## ILI9341 TFT Display

| Symptom | Cause | Fix |
|---------|-------|-----|
| Blank screen | Not powered or wrong wiring | Verify 3.3V VCC, GND, CS/DC/RST pins |
| **Dead display** | Connected to 5V | **Hardware damage** - replace display |
| Garbled pixels | Wrong rotation or bounds | `setRotation(1)`; don't draw outside 320×240 |
| Flickering vitals | Full screen refresh | Use `setTextColor(fg, bg)` overwrite pattern |
| Choppy ECG + sensors | TFT blocks too long | Limit ECG draw rate (`ECG_DRAW_MS` in config.h) |
| White noise on SPI | No level shifting | Add 1kΩ series on SPI from 5V Arduino |

---

## SIM800L GSM

| Symptom | Cause | Fix |
|---------|-------|-----|
| Module resets | Insufficient power | 4V 2A supply; thick wires; 1000µF cap near module |
| No AT response | Wrong baud or wiring | 9600 baud; check TX divider to RX |
| +CPIN ERROR | SIM issue | Insert SIM; unlock PIN; use 2G-capable card |
| CREG not 1 or 5 | No network | Attach antenna; check coverage; wait outdoors |
| GPRS fails | Wrong APN | Set `GSM_APN` in config.h for your carrier |
| HTTP timeout | URL or bearer | Verify `GSM_SERVER_URL`; SAPBR open returns OK |
| Garbage responses | Buffer overflow | Drain serial between commands; non-blocking parser |

---

## Arduino → Raspberry Pi UART

| Symptom | Cause | Fix |
|---------|-------|-----|
| No data on Pi | Console still on serial | Remove `console=serial0` from cmdline.txt |
| Garbage characters | Baud mismatch | Both sides 9600 baud |
| Pi receives nothing | Bluetooth owns UART | `dtoverlay=disable-bt`; use `/dev/serial0` |
| Pi GPIO damaged | 5V into RX | Add voltage divider on Arduino TX |
| Wrong device file | Using miniUART | Prefer `/dev/serial0` → ttyAMA0 |

---

## MQTT / Mosquitto (Raspberry Pi)

| Symptom | Cause | Fix |
|---------|-------|-----|
| Connection refused | Broker not running | `sudo systemctl status mosquitto` |
| TLS handshake fail | Wrong CA path | Python uses `ca.crt`, not `server.crt` |
| Certificate verify failed | CN mismatch | Regenerate cert with CN=localhost or Pi hostname |
| Auth failed | Password mismatch | Match config.yaml and `/etc/mosquitto/passwd` |
| Messages not arriving | Publish before connect | Wait for `on_connect` before publish |
| No messages after disconnect | loop not running | Use `loop_start()` in mqtt_client |
| Permission denied on certs | File ownership | `chown mosquitto:mosquitto /etc/mosquitto/certs/*` |
| Broker won't start | Bad config paths | Check mosquitto.log; verify cert file paths |

---

## Python Gateway

| Symptom | Cause | Fix |
|---------|-------|-----|
| Malformed line warnings | Wrong CSV format | Expect `HR:n,SPO2:n,TEMP:n.n,ECG:n` |
| Serial permission denied | User not in dialout | `sudo usermod -aG dialout pi` |
| Gateway exits on error | Unhandled exception | Check logs; run `python3 gateway.py` manually |
| No MQTT publish | Serial blocking | `timeout=1.0` in config.yaml |

---

## Phase 1 Isolation Tests

Run sketches in `arduino/tests/` one at a time before full integration:

1. `test_ecg` - Serial plotter @ 115200
2. `test_max30100` - HR/SpO2 every second
3. `test_mlx90614` - Object/ambient temps
4. `test_tft` - Color bars on screen
5. `test_sim800l` - AT passthrough from Serial Monitor

Pi tests (no Arduino):

```bash
python3 raspberry_pi/scripts/test_serial.py
python3 raspberry_pi/scripts/test_mqtt_publish.py
```
