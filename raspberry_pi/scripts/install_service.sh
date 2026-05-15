#!/bin/bash
# Install systemd service for medical gateway
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GATEWAY_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
USER_NAME="${SUDO_USER:-pi}"

if [[ $EUID -ne 0 ]]; then
  echo "Run as root: sudo $0"
  exit 1
fi

cat > /etc/systemd/system/medical-gateway.service <<EOF
[Unit]
Description=Portable Emergency Medical Kit MQTT Gateway
After=network.target mosquitto.service
Wants=mosquitto.service

[Service]
Type=simple
User=$USER_NAME
WorkingDirectory=$GATEWAY_DIR
ExecStart=/usr/bin/python3 $GATEWAY_DIR/gateway.py
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable medical-gateway.service

echo "Service installed. Commands:"
echo "  sudo systemctl start medical-gateway"
echo "  sudo systemctl status medical-gateway"
echo "  journalctl -u medical-gateway -f"
