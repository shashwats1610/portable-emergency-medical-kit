#!/bin/bash
# Install and configure Mosquitto with TLS for medical kit
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

if [[ $EUID -ne 0 ]]; then
  echo "Run as root: sudo $0"
  exit 1
fi

echo "Installing Mosquitto..."
apt-get update
apt-get install -y mosquitto mosquitto-clients openssl

echo "Generating certificates..."
bash "$SCRIPT_DIR/generate_certs.sh" /etc/mosquitto/certs

echo "Installing Mosquitto config..."
cp "$PROJECT_ROOT/mosquitto/mosquitto.conf" /etc/mosquitto/conf.d/medical_kit.conf

if [[ ! -f /etc/mosquitto/passwd ]]; then
  echo "Create MQTT user 'medical_gateway':"
  mosquitto_passwd -c /etc/mosquitto/passwd medical_gateway
fi

systemctl enable mosquitto
systemctl restart mosquitto

echo "Mosquitto TLS broker running on port 8883"
echo "Test subscribe:"
echo "  mosquitto_sub -h 127.0.0.1 -p 8883 --cafile /etc/mosquitto/certs/ca.crt \\"
echo "    -u medical_gateway -P YOUR_PASSWORD -t medical_kit/vitals -v"
