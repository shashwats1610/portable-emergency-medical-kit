#!/bin/bash
# Configure Raspberry Pi UART for Arduino gateway
set -euo pipefail

echo "=== Raspberry Pi UART Setup ==="
echo ""
echo "Manual steps required:"
echo ""
echo "1. Run: sudo raspi-config"
echo "   Interface Options -> Serial Port"
echo "   - Login shell over serial?  -> No"
echo "   - Serial hardware enabled?  -> Yes"
echo ""
echo "2. Edit /boot/firmware/config.txt (or /boot/config.txt on older Pi):"
echo "   enable_uart=1"
echo "   dtoverlay=disable-bt"
echo ""
echo "3. Edit /boot/firmware/cmdline.txt (or /boot/cmdline.txt):"
echo "   Remove: console=serial0,115200"
echo ""
echo "4. Reboot: sudo reboot"
echo ""
echo "5. Wire Arduino TX (5V) -> voltage divider -> Pi GPIO15 (RX)"
echo "   Pi GPIO14 (TX) -> Arduino RX (direct, 3.3V safe)"
echo "   Common GND between Pi and Arduino"
echo ""
echo "6. Verify device: ls -l /dev/serial0"
echo "   Should point to ttyAMA0 on Pi 3/4 with Bluetooth disabled"
echo ""

CONFIG_TXT=""
if [[ -f /boot/firmware/config.txt ]]; then
  CONFIG_TXT=/boot/firmware/config.txt
elif [[ -f /boot/config.txt ]]; then
  CONFIG_TXT=/boot/config.txt
fi

if [[ -n "$CONFIG_TXT" && $EUID -eq 0 ]]; then
  grep -q '^enable_uart=1' "$CONFIG_TXT" || echo 'enable_uart=1' >> "$CONFIG_TXT"
  grep -q 'disable-bt' "$CONFIG_TXT" || echo 'dtoverlay=disable-bt' >> "$CONFIG_TXT"
  echo "Appended UART settings to $CONFIG_TXT - reboot required"
fi
