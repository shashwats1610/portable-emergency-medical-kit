#!/usr/bin/env python3
"""Publish a single test vitals message to local Mosquitto TLS broker."""

import sys
import time
from pathlib import Path

import yaml

sys.path.insert(0, str(Path(__file__).parent.parent))

from mqtt_client import MqttVitalsClient

CONFIG_PATH = Path(__file__).parent.parent / "config.yaml"


def main():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f)

    mqtt_cfg = cfg["mqtt"]
    client = MqttVitalsClient(
        host=mqtt_cfg["host"],
        port=mqtt_cfg["port"],
        topic=mqtt_cfg["topic"],
        qos=mqtt_cfg["qos"],
        ca_cert=mqtt_cfg["ca_cert"],
        username=mqtt_cfg["username"],
        password=mqtt_cfg["password"],
    )

    client.connect()
    time.sleep(1)

    ok = client.publish_vitals({
        "hr": 72,
        "spo2": 98,
        "temp": 36.6,
        "ecg": 512,
    })
    print("Publish OK" if ok else "Publish FAILED")
    client.disconnect()
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
