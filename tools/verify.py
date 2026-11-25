"""Validate a source-only copy and keep test firmware in a separate build tree."""
import argparse
import base64
import fcntl
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / ".verify"
TARGETS = ("outdoor-string-lights", "rf-receive", "rf-survey", "rf-receive-lan")


def verify(build):
    esphome = ROOT / ".venv-esphome/bin/esphome"
    for folder in ("packages", "src"):
        dest = WORK / folder
        if dest.exists():
            shutil.rmtree(dest)
        shutil.copytree(ROOT / folder, dest)
    for name in ("platformio.ini", *(f"{t}.yaml" for t in TARGETS)):
        shutil.copy2(ROOT / name, WORK / name)

    secrets = WORK / "secrets.yaml"
    # Stable dummy values keep incremental builds reusable.
    key = base64.b64encode(bytes(range(32))).decode()
    secrets.write_text(f"wifi_ssid: test-network\nwifi_password: test-only\nstring_lights_api_key: {key}\n")
    resolved = WORK / "resolved-controller.yaml"
    try:
        for target in TARGETS:
            result = subprocess.run([esphome, "config", target + ".yaml"], cwd=WORK, stdout=subprocess.PIPE)
            if result.returncode:
                sys.stderr.buffer.write(result.stdout)
                result.check_returncode()
            if target == TARGETS[0]:
                resolved.write_bytes(result.stdout)
        subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"],
                       cwd=ROOT, env={**os.environ, "RF_TEST_CONFIG": str(resolved)}, check=True)
        if build:
            for target in TARGETS:
                subprocess.run([esphome, "compile", target + ".yaml"], cwd=WORK, check=True)
            subprocess.run([ROOT / ".venv/bin/pio", "run"], cwd=WORK, check=True)
    finally:
        secrets.unlink(missing_ok=True)
        resolved.unlink(missing_ok=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="store_true")
    WORK.mkdir(exist_ok=True)
    with (WORK / "verify.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        verify(parser.parse_args().build)
