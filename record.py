"""Record USB listener output with host timestamps, including quiet periods."""
import argparse
from datetime import datetime
from pathlib import Path
import re
import time

import serial

ANSI_COLOR = re.compile(rb"\x1b\[[0-9;]*m")


def write_line(log, data):
    message = ANSI_COLOR.sub(b"", data).rstrip(b"\r").decode(errors="replace")
    print(datetime.now().isoformat(timespec="milliseconds"), message, file=log)


def record(port, log, duration):
    deadline = time.monotonic() + duration
    last_activity = time.monotonic()
    partial = bytearray()
    try:
        while time.monotonic() < deadline:
            # Drain available bytes together. readline() on pySerial reads one byte at a time.
            data = port.read(port.in_waiting or 1)
            now = time.monotonic()
            if data:
                lines = data.split(b"\n")
                for line in lines[:-1]:
                    partial.extend(line)
                    write_line(log, partial)
                    partial.clear()
                partial.extend(lines[-1])
                last_activity = now
            elif now - last_activity >= 4:
                if partial:
                    write_line(log, partial)
                    partial.clear()
                write_line(log, b"Host received no serial output for four seconds")
                last_activity = now
    finally:
        if partial:
            write_line(log, partial)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port")
    parser.add_argument("label", help="short filename label, e.g. fan-on")
    args = parser.parse_args()
    if not args.label or Path(args.label).name != args.label:
        parser.error("Use a capture label without a directory path.")
    directory = Path("captures")
    directory.mkdir(exist_ok=True)
    path = directory / f"{args.label}-{datetime.now():%Y%m%d-%H%M%S}.log"
    try:
        with serial.Serial(args.port, 115200, timeout=1) as port, path.open("x", buffering=1) as log:
            port.dtr = False
            port.rts = True
            time.sleep(0.15)
            port.rts = False
            print(f"Recording to {path}", flush=True)
            record(port, log, duration=600)
    except KeyboardInterrupt:
        print(f"Stopped recording to {path}")


if __name__ == "__main__":
    main()
