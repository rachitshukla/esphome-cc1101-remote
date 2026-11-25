"""Record the listening board's USB output, including quiet periods."""
from datetime import datetime
from pathlib import Path
import re
import sys
import time

import serial

port_name, label = sys.argv[1:3]
directory = Path("captures")
directory.mkdir(exist_ok=True)
path = directory / f"{label}-{datetime.now():%Y%m%d-%H%M%S}.log"
with serial.Serial(port_name, 115200, timeout=1) as port, path.open("x", buffering=1) as log:
    print(f"Recording to {path}", flush=True)
    port.dtr = False
    port.rts = True
    time.sleep(0.15)
    port.rts = False
    deadline = time.monotonic() + 600
    last_line = time.monotonic()
    while time.monotonic() < deadline:
        data = port.readline()
        if data:
            last_line = time.monotonic()
            message = re.sub(r"\x1b\[[0-9;]*m", "", data.decode(errors="replace").rstrip())
        elif time.monotonic() - last_line > 4:
            message = "Host received no serial output for four seconds"
            last_line = time.monotonic()
        else:
            continue
        print(datetime.now().isoformat(timespec="milliseconds"), message, file=log)
