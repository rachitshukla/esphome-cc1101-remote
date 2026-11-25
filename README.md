# ESPHome CC1101 Remote

Control sub-GHz RF devices from Home Assistant with an ESP32 and a CC1101 radio. Home Assistant sends commands over the local network using ESPHome’s encrypted native API. The ESP32 then transmits the RF command.

The outdoor string lights example grew out of a working two-button remote. Its ON and OFF messages decoded at **433.92 MHz, ASK/OOK, RC Switch protocol 1**. Each button produced the same 32-bit message across five independent presses. The configuration includes the measured ON and OFF codes from that remote. Capture your own codes when adapting it to another device. Home Assistant displays the last commanded state. The lights provide no feedback when someone uses the handheld remote.

## Hardware

The tested setup uses an ESP32-WROOM-32 development board, a CC1101 module with an antenna, and these connections. Power the radio at **3.3 V**.

| CC1101 pin | ESP32 connection |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO18 |
| SI / MOSI | GPIO23 |
| SO / MISO | GPIO19 |
| CSN | GPIO5 |
| GDO0 (transmit data) | GPIO4 |
| GDO2 (receive data) | GPIO27 |

## Build and check

Install Git, Make, a C++17 compiler, and Python. The tested Intel Mac used Python 3.13.13 for ESPHome and Python 3.9.6 for the PlatformIO diagnostic.

```sh
git clone https://github.com/rachitshukla/esphome-cc1101-remote.git
cd esphome-cc1101-remote
make setup
make check
make verify
```

Override `ESPHOME_PYTHON` or `DIAGNOSTIC_PYTHON` when running `make setup` if your Python executables have different names. `make check` validates the configurations and runs the host tests. `make verify` also builds all five firmware targets sequentially. Verification uses dummy credentials and writes build artifacts under `.verify/`.

## Find and capture a remote

| Target | When to use it |
| --- | --- |
| [SPI diagnostic](src/main.cpp) | Check the radio’s identity registers and verify repeated SPI reads and writes. |
| [Frequency survey](rf-survey.yaml) | Compare signal strength on candidate frequencies while pressing the remote. |
| [USB listener](rf-receive.yaml) | Record pulse timings and try RC Switch decoding at one frequency. |
| [LAN listener](rf-receive-lan.yaml) | Change the listening frequency through Home Assistant’s `tune_receiver` action. |
| [Lights controller](outdoor-string-lights.yaml) | Send a verified ON or OFF command from a Home Assistant light entity. |

Read the model and frequency label on your remote first. Start with the SPI diagnostic, then listen at the labeled frequency. If it is unknown, edit the survey’s candidate list. Compare a quiet baseline with several presses of each button. Use `record.py` to keep timestamped USB logs. Installing a listener or diagnostic replaces the firmware currently on that board.

For example, to capture at 315 MHz on a Mac, set `PORT` to the board’s USB serial device. The upload command **writes firmware to the board**.

```sh
PORT=/dev/cu.usbserial-0001
.venv-esphome/bin/esphome -s rx_frequency 315MHz compile rf-receive.yaml
.venv-esphome/bin/esphome -s rx_frequency 315MHz upload rf-receive.yaml --device "$PORT"
.venv/bin/python record.py "$PORT" remote-on
```

## Connect a device to Home Assistant

1. Set the frequency, protocol, repeat count, and ON/OFF codes in the lights configuration to the values you verified. Preserve leading zeros in the binary codes.
2. Create an untracked `secrets.yaml` next to the configuration with `wifi_ssid`, `wifi_password`, and `string_lights_api_key`. Generate the API key locally with `openssl rand -base64 32`.
3. Build and install the controller using `.venv-esphome/bin/esphome run outdoor-string-lights.yaml --device "$PORT"`.
4. Add the ESPHome integration in Home Assistant. Select the discovered device or enter `string-lights-radio.local`, then enter the API key. Test ON and OFF while observing the lights.

For another compatible fixed-code device, reuse the [shared transmit action](packages/send-rc-switch.yaml) with its measured frequency, protocol, repeat count, and command. The controller can switch frequency between devices. Fan commands depend on the specific remote and require their own captures.
