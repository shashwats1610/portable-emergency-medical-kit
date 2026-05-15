"""MQTT client wrapper. TLS + reconnect with backoff."""

import json
import logging
import threading
import time
from typing import Any, Optional

import paho.mqtt.client as mqtt

logger = logging.getLogger(__name__)


class MqttVitalsClient:
    def __init__(
        self,
        host: str,
        port: int,
        topic: str,
        qos: int,
        ca_cert: str,
        username: str,
        password: str,
    ):
        self.host = host
        self.port = port
        self.topic = topic
        self.qos = qos
        self.ca_cert = ca_cert
        self.username = username
        self.password = password
        self._connected = False
        self._backoff = 1
        self._lock = threading.Lock()
        self._needs_reconnect = False

        try:
            self.client = mqtt.Client(
                client_id="medical_gateway",
                callback_api_version=mqtt.CallbackAPIVersion.VERSION1,
            )
        except AttributeError:
            self.client = mqtt.Client(client_id="medical_gateway")
        self.client.username_pw_set(username, password)
        self.client.tls_set(ca_certs=ca_cert)
        self.client.tls_insecure_set(False)

        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect

    def _on_connect(self, client, userdata, flags, rc):
        with self._lock:
            if rc == 0:
                self._connected = True
                self._backoff = 1
                self._needs_reconnect = False
                logger.info("MQTT connected to %s:%d", self.host, self.port)
            else:
                self._connected = False
                logger.error("MQTT connect failed rc=%s", rc)

    def _on_disconnect(self, client, userdata, rc):
        with self._lock:
            self._connected = False
            if rc != 0:
                self._needs_reconnect = True
                logger.warning("MQTT disconnected rc=%s - will reconnect", rc)

    def connect(self) -> None:
        while True:
            try:
                self.client.connect(self.host, self.port, keepalive=60)
                self.client.loop_start()
                for _ in range(50):
                    with self._lock:
                        if self._connected:
                            return
                    time.sleep(0.1)
                raise ConnectionError("MQTT connect timeout")
            except Exception as exc:
                logger.error(
                    "MQTT connect error: %s - retry in %ds", exc, self._backoff
                )
                time.sleep(self._backoff)
                self._backoff = min(self._backoff * 2, 60)

    def ensure_connected(self) -> bool:
        with self._lock:
            if self._connected:
                return True
            needs = self._needs_reconnect

        if not needs:
            return False

        logger.info("MQTT reconnecting (backoff %ds)...", self._backoff)
        try:
            self.client.reconnect()
            for _ in range(30):
                with self._lock:
                    if self._connected:
                        return True
                time.sleep(0.1)
        except Exception as exc:
            logger.error("MQTT reconnect failed: %s", exc)
            time.sleep(self._backoff)
            self._backoff = min(self._backoff * 2, 60)
            with self._lock:
                self._needs_reconnect = True

        return False

    def publish_vitals(self, vitals: dict[str, Any]) -> bool:
        if not self._connected:
            if not self.ensure_connected():
                logger.warning("MQTT not connected, skipping publish")
                return False

        payload = {
            "hr": vitals.get("hr"),
            "spo2": vitals.get("spo2"),
            "temp": vitals.get("temp"),
            "ecg": vitals.get("ecg"),
            "timestamp": int(time.time()),
        }

        result = self.client.publish(
            self.topic,
            json.dumps(payload),
            qos=self.qos,
        )
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            logger.error("MQTT publish failed rc=%s", result.rc)
            return False

        if self.qos > 0:
            try:
                result.wait_for_publish(timeout=2.0)
            except Exception as exc:
                logger.warning("MQTT publish ack timeout: %s", exc)
                return False

        logger.debug("Published: %s", payload)
        return True

    def disconnect(self) -> None:
        self.client.loop_stop()
        time.sleep(0.5)
        self.client.disconnect()
        with self._lock:
            self._connected = False
        logger.info("MQTT disconnected")

    @property
    def is_connected(self) -> bool:
        with self._lock:
            return self._connected
