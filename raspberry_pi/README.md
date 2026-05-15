# Raspberry Pi Gateway

UART gateway that reads Arduino telemetry and publishes JSON to a local Mosquitto broker over TLS.

## Quick Start

```bash
cd raspberry_pi
pip install -r requirements.txt

# 1. Configure UART (see scripts/setup_uart.sh)
sudo bash scripts/setup_uart.sh
sudo reboot

# 2. Install Mosquitto + TLS certs
sudo bash scripts/setup_mosquitto.sh
# Set password in config.yaml to match mosquitto_passwd

# 3. Run gateway
python3 gateway.py

# 4. Optional: install as service
sudo bash scripts/install_service.sh
sudo systemctl start medical-gateway
```

## Configuration

Edit `config.yaml`:

- `serial.port`: `/dev/serial0` (after UART enabled, console disabled)
- `serial.baud`: `9600` (must match Arduino)
- `mqtt.password`: must match `/etc/mosquitto/passwd`

## Tests

```bash
python3 scripts/test_serial.py
python3 scripts/test_mqtt_publish.py
```

## Subscribe to vitals

```bash
mosquitto_sub -h 127.0.0.1 -p 8883 \
  --cafile /etc/mosquitto/certs/ca.crt \
  -u medical_gateway -P YOUR_PASSWORD \
  -t medical_kit/vitals -v
```

## MQTT Payload

```json
{
  "hr": 75,
  "spo2": 98,
  "temp": 36.5,
  "ecg": 512,
  "timestamp": 1640000000
}
```
