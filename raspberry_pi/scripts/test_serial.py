#!/usr/bin/env python3
"""Test serial parser with sample CSV lines (no hardware required)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from serial_reader import TELEMETRY_PATTERN

SAMPLES = [
    "HR:75,SPO2:98,TEMP:36.5,ECG:512",
    "HR:-1,SPO2:-1,TEMP:-1,ECG:-1",
    "garbage line",
]

for line in SAMPLES:
    m = TELEMETRY_PATTERN.match(line)
    if m:
        print(f"OK: {line} -> {m.groupdict()}")
    else:
        print(f"SKIP: {line}")
