"""Read CSV vitals lines from Arduino UART."""

import logging
import re
from typing import Optional

import serial

logger = logging.getLogger(__name__)

TELEMETRY_PATTERN = re.compile(
    r"HR:(?P<hr>-?\d+),SPO2:(?P<spo2>-?\d+),TEMP:(?P<temp>-?\d+(?:\.\d+)?),ECG:(?P<ecg>-?\d+)"
)

HR_MIN, HR_MAX = 40, 200
SPO2_MIN, SPO2_MAX = 70, 100
TEMP_MIN, TEMP_MAX = 25.0, 45.0
SERIAL_BACKLOG_BYTES = 256


class SerialReader:
    def __init__(self, port: str, baud: int, timeout: float):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self._serial: Optional[serial.Serial] = None

    def open(self) -> None:
        self._serial = serial.Serial(
            port=self.port,
            baudrate=self.baud,
            timeout=self.timeout,
        )
        logger.info("Opened serial port %s @ %d baud", self.port, self.baud)

    def close(self) -> None:
        if self._serial and self._serial.is_open:
            self._serial.close()
            logger.info("Closed serial port %s", self.port)

    def _flush_backlog(self) -> None:
        if self._serial and self._serial.in_waiting > SERIAL_BACKLOG_BYTES:
            logger.warning(
                "Serial backlog %d bytes - flushing stale data",
                self._serial.in_waiting,
            )
            self._serial.reset_input_buffer()

    def read_vitals(self) -> Optional[dict]:
        if not self._serial or not self._serial.is_open:
            raise RuntimeError("Serial port not open")

        self._flush_backlog()

        raw = self._serial.readline()
        if not raw:
            return None

        if not raw.endswith(b"\n"):
            logger.debug("Discarding partial line (no newline)")
            return None

        text = raw.decode("utf-8", errors="ignore").strip()
        if not text:
            return None

        match = TELEMETRY_PATTERN.match(text)
        if not match:
            logger.warning("Malformed telemetry line: %s", text)
            return None

        vitals = {
            "hr": self._parse_int(match.group("hr")),
            "spo2": self._parse_int(match.group("spo2")),
            "temp": self._parse_float(match.group("temp")),
            "ecg": self._parse_int(match.group("ecg")),
        }

        return self._validate_ranges(vitals, text)

    @staticmethod
    def _parse_int(value: str) -> Optional[int]:
        iv = int(value)
        return iv if iv >= 0 else None

    @staticmethod
    def _parse_float(value: str) -> Optional[float]:
        fv = float(value)
        return fv if fv >= 0 else None

    def _validate_ranges(self, vitals: dict, raw_line: str) -> Optional[dict]:
        hr = vitals.get("hr")
        if hr is not None and not (HR_MIN <= hr <= HR_MAX):
            logger.warning("HR out of range (%s): %s", hr, raw_line)
            vitals["hr"] = None

        spo2 = vitals.get("spo2")
        if spo2 is not None and not (SPO2_MIN <= spo2 <= SPO2_MAX):
            logger.warning("SpO2 out of range (%s): %s", spo2, raw_line)
            vitals["spo2"] = None

        temp = vitals.get("temp")
        if temp is not None and not (TEMP_MIN <= temp <= TEMP_MAX):
            logger.warning("Temp out of range (%s): %s", temp, raw_line)
            vitals["temp"] = None

        return vitals
