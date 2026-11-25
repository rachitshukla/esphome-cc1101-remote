"""Exercise ESPHome's resolved device configuration without a USB device."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT / "tests"


class RadioTest(unittest.TestCase):
    def test_controller(self):
        # make check validates against isolated test credentials and emits this file.
        profile = yaml.load(Path(os.environ["RF_TEST_CONFIG"]).read_text(), Loader=yaml.BaseLoader)
        expected = json.loads((TESTS / "fixtures/string-lights.json").read_text())
        self.assertEqual(float(profile["cc1101"][0]["frequency"]), expected["frequency_hz"])
        light = profile["light"][0]
        self.assertEqual(light["name"], "Outdoor String Lights")
        output = next(o for o in profile["output"] if o["id"] == light["output"])
        guard, = output["write_action"]["then"]
        stage, send = guard["if"]["then"]
        self.assertEqual(stage["globals.set"]["id"], "next_tx_hz")
        frame = send["remote_transmitter.transmit_rc_switch_raw"]
        self.assertEqual(int(frame["protocol"]), expected["protocol"])
        self.assertEqual(int(frame["repeat"]["times"]), expected["repeats"])
        self.assertEqual(frame["repeat"]["wait_time"], f"{expected['wait_us']}us")

        boot, = profile["esphome"]["on_boot"]
        tx, = profile["remote_transmitter"]
        self.assertEqual(tx["non_blocking"], "true")
        actions = {"cc1101.set_idle": "device.events.push_back(idle);",
                   "cc1101.begin_tx": "device.events.push_back(transmit);"}

        def callback(steps):
            body = []
            for step in steps:
                (name, value), = step.items()
                body.append(value if name == "lambda" else actions[name])
            return "\n".join(body)

        init = {g["id"]: g["initial_value"] for g in profile["globals"]}
        callbacks = f"""
bool ready_for_commands = {init['ready_for_commands']};
uint32_t next_tx_hz = {init['next_tx_hz']};
bool enabled() {{ {guard['if']['condition']['lambda']} }}
void boot() {{ {callback(boot['then'])} }}
uint32_t lights_frequency() {{ {stage['globals.set']['value']} }}
std::string code(bool state) {{ {frame['code']} }}
void start_frame() {{ {callback(tx['on_transmit']['then'])} }}
void finish_frame() {{ {callback(tx['on_complete']['then'])} }}
"""
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)
            (path / "callbacks.inc").write_text(callbacks)
            (path / "expected.h").write_text(
                f"constexpr unsigned expected_hz = {expected['frequency_hz']};\n"
                f"constexpr auto expected_on = {json.dumps(expected['on'])};\n"
                f"constexpr auto expected_off = {json.dumps(expected['off'])};\n"
            )
            exe = path / "radio-test"
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-include", str(path / "expected.h"),
                            "-I", str(path), "-I", str(ROOT / "packages"), str(TESTS / "radio_sim.cpp"),
                            "-o", str(exe)], check=True)
            subprocess.run([str(exe)], cwd=path, check=True)
