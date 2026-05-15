#!/usr/bin/env python3
"""Pi gateway: serial CSV from Arduino -> MQTT JSON."""

import logging
import os
import signal
import sys
import time
from pathlib import Path

import yaml

from mqtt_client import MqttVitalsClient
from serial_reader import SerialReader

CONFIG_PATH = Path(__file__).parent / "config.yaml"
running = True


def load_config(path: Path) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def resolve_mqtt_password(cfg: dict) -> str:
    env_pass = os.getenv("MQTT_PASSWORD")
    if env_pass:
        return env_pass
    mqtt_pass = cfg.get("mqtt", {}).get("password")
    if not mqtt_pass or mqtt_pass == "changeme":
        logging.warning(
            "Using default MQTT password - set MQTT_PASSWORD env var for production"
        )
    return mqtt_pass


def setup_logging(cfg: dict) -> None:
    level = getattr(logging, cfg.get("logging", {}).get("level", "INFO"))
    handlers = [logging.StreamHandler(sys.stdout)]

    log_file = cfg.get("logging", {}).get("file")
    if log_file:
        try:
            handlers.append(logging.FileHandler(log_file))
        except OSError as exc:
            print(f"Warning: cannot open log file {log_file}: {exc}")

    logging.basicConfig(
        level=level,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        handlers=handlers,
    )


def handle_signal(signum, frame):
    global running
    logging.info("Shutdown signal received")
    running = False


def main() -> int:
    cfg = load_config(CONFIG_PATH)
    setup_logging(cfg)

    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    serial_cfg = cfg["serial"]
    mqtt_cfg = cfg["mqtt"]

    reader = SerialReader(
        port=serial_cfg["port"],
        baud=serial_cfg["baud"],
        timeout=serial_cfg["timeout"],
    )

    mqtt_client = MqttVitalsClient(
        host=mqtt_cfg["host"],
        port=mqtt_cfg["port"],
        topic=mqtt_cfg["topic"],
        qos=mqtt_cfg["qos"],
        ca_cert=mqtt_cfg["ca_cert"],
        username=mqtt_cfg["username"],
        password=resolve_mqtt_password(cfg),
    )

    reader.open()
    mqtt_client.connect()

    log = logging.getLogger("gateway")
    log.info("Gateway running")

    try:
        while running:
            if not mqtt_client.is_connected:
                mqtt_client.ensure_connected()

            vitals = reader.read_vitals()
            if vitals:
                mqtt_client.publish_vitals(vitals)
            else:
                time.sleep(0.01)
    finally:
        reader.close()
        mqtt_client.disconnect()
        log.info("Gateway stopped")

    return 0


if __name__ == "__main__":
    sys.exit(main())
