# Testing Notes

Bench procedures from actual bring-up order. Not a generic checklist.

---

## Expected value ranges

| Reading | Normal range | Invalid / investigate |
|---------|--------------|------------------------|
| HR | 40-200 BPM | 0, <40, >200 |
| SpO2 | 70-100 % | <70 (hypoxic), >100 (sensor error) |
| Object temp | 25-45 C (body/apple at ~10 cm) | <25 or >45 at room temp |
| ECG ADC | ~200-800 with electrodes on forearms | 0-50 or 1020+ with leads on |
| UART line | `HR:75,SPO2:98,TEMP:36.5,ECG:512` | missing fields, no newline |

---

## Phase 1 - isolate each sensor

### test_ecg (`arduino/tests/test_ecg/`)
1. Upload, Serial Monitor **115200** baud.
2. Attach RA/LA/RL electrodes.
3. **Expect:** `ECG:400-700` varying rhythmically when leads on.
4. Remove one electrode - **expect:** `LEAD OFF` within one line.
5. **Fail:** constant 0 or 1023 - check LO+ pin 6, LO- pin 7, OUTPUT on A0.

### test_max30100
1. Serial 115200, finger over sensor window.
2. Wait 10-15 s for perfusion.
3. **Expect:** HR 60-100 at rest, SpO2 95-100.
4. **Fail init:** I2C scan empty at 0x57 - pull-up fix (see `docs/WIRING.md`).

### test_mlx90614
1. Point at palm from 5 cm.
2. **Expect:** Object temp 30-36 C, ambient 20-28 C indoors.
3. **Fail:** 0 or -273 - wiring or wrong address.

### test_tft
1. **3.3V only** on VCC before power.
2. **Expect:** "TFT OK" + four color bars.
3. White screen = wiring; black = no SPI / wrong CS pin.

### test_sim800l
1. External 4 V 2 A, antenna on, SIM inserted.
2. Open Serial 115200, should see `OK` after boot `AT`.
3. Type `AT+CREG?` manually - look for `+CREG: 0,1` or `,5`.

---

## Phase 2 - integrated firmware

### Upload `arduino/main.ino`
1. Serial **9600** (matches Pi).
2. Boot log: each sensor OK or FAILED line.
3. **Expect** telemetry every 500 ms:
   ```
   HR:72,SPO2:97,TEMP:36.4,ECG:512
   ```
4. Invalid finger: `HR:-1` or out-of-range filtered to -1 on Arduino.

### ECG sample rate check (optional sketch tweak)
Add temporarily in `sensorManagerUpdate()` after ECG sample:
```cpp
static unsigned long lastHzPrint;
static uint16_t ecgSamples;
ecgSamples++;
if (millis() - lastHzPrint >= 1000) {
  Serial.print(F("ECG Hz: ")); Serial.println(ecgSamples);
  ecgSamples = 0;
  lastHzPrint = millis();
}
```
- **Target:** ~200 Hz (ECG_SAMPLE_MS=5).
- **Accept:** 150+ Hz with TFT connected (display steals loop time).

### TFT integration
- Vitals update without full-screen flicker.
- ECG trace scrolls when leads on; status shows `LEAD OFF` when not.
- If loop feels sluggish, ECG graph redraw is the culprit (~33 ms interval).

---

## Phase 3 - Raspberry Pi gateway

### UART wiring
- Arduino TX -> 10k -> Pi RX -> 20k -> GND.
- Pi TX -> Arduino RX direct.
- Common GND.

### Pi UART enable
```bash
sudo bash raspberry_pi/scripts/setup_uart.sh
sudo reboot
ls -l /dev/serial0   # should exist
```

### Raw serial sniff
```bash
stty -F /dev/serial0 9600 raw -echo
cat /dev/serial0
```
**Expect:** CSV lines every 500 ms. Garbage = console still on serial or wrong baud.

### Mosquitto TLS
```bash
sudo bash raspberry_pi/scripts/setup_mosquitto.sh
cp raspberry_pi/config.yaml.example raspberry_pi/config.yaml
# edit password OR: export MQTT_PASSWORD=yourpass
```

### Gateway + subscribe
```bash
cd raspberry_pi
pip install -r requirements.txt
python3 gateway.py
# other terminal:
mosquitto_sub -h 127.0.0.1 -p 8883 --cafile /etc/mosquitto/certs/ca.crt \
  -u medical_gateway -P YOURPASS -t medical_kit/vitals -v
```
**Expect:** JSON with unix timestamp every ~500 ms.

### Parser unit test (no hardware)
```bash
python3 scripts/test_serial.py
```

### Broker restart test
1. Run gateway.
2. `sudo systemctl restart mosquitto`
3. **Expect:** gateway logs reconnect within ~60 s max; vitals resume.

---

## Phase 4 - GSM (ENABLE_GSM 1)

Only after UART path works.

1. Set `ENABLE_GSM` to 1 in `config.h`, set `GSM_APN` for your carrier.
2. Power SIM800L from buck 4.0 V - measure 2 A capability under TX burst.
3. Serial log should reach `[GSM] Ready for HTTP`.
4. **Bearer stuck test:** upload sketch twice without power-cycling SIM800L - should still reach READY (SAPBR close retry).

---

## Common failure modes (quick ref)

| What you see | Likely cause | Doc |
|--------------|--------------|-----|
| MAX30100 init fail | RCWL-0530 pull-ups | WIRING.md |
| TFT dead | 5V on VCC | WIRING.md |
| Pi garbage UART | console on serial0 | TROUBLESHOOTING.md |
| MQTT TLS fail | wrong CA file / password | TROUBLESHOOTING.md |
| GSM SAPBR ERROR | bearer already open | fixed in gsm_manager - power cycle if old firmware |
| HR:-1 always | finger off or range filter | normal until finger placed |

---

## Pre-demo checklist

- [ ] TFT on 3.3V
- [ ] Pi voltage divider on Arduino TX
- [ ] SIM800L on external supply (if used)
- [ ] `config.yaml` not committed with real password
- [ ] `mosquitto` running: `systemctl is-active mosquitto`
- [ ] Finger on MAX30100 for live HR/SpO2
