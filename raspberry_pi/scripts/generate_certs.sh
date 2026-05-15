#!/bin/bash
# Generate self-signed CA and server certificates for Mosquitto TLS
set -euo pipefail

CERT_DIR="${1:-/etc/mosquitto/certs}"
DAYS=3650

if [[ $EUID -ne 0 ]]; then
  echo "Run as root: sudo $0 [cert_dir]"
  exit 1
fi

mkdir -p "$CERT_DIR"
cd "$CERT_DIR"

echo "Generating CA key and certificate..."
openssl req -new -x509 -days "$DAYS" -extensions v3_ca \
  -keyout ca.key -out ca.crt \
  -subj "/CN=MedicalKit-CA/O=PortableMedicalKit/C=US"

chmod 600 ca.key
chmod 644 ca.crt

echo "Generating server key..."
openssl genrsa -out server.key 2048
chmod 600 server.key

echo "Generating server CSR..."
openssl req -new -out server.csr -key server.key \
  -subj "/CN=localhost/O=PortableMedicalKit/C=US"

echo "Signing server certificate..."
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days "$DAYS"
chmod 644 server.crt

rm -f server.csr

chown -R mosquitto:mosquitto "$CERT_DIR" 2>/dev/null || true

echo "Certificates created in $CERT_DIR"
echo "  ca.crt   - use in Python mqtt_client ca_cert"
echo "  server.crt / server.key - Mosquitto listener 8883"
