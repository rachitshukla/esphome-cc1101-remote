# ESP32 and CC1101 SPI diagnostic

The firmware resets the radio, reads configuration and status registers,
checks 32 repeated identity reads, then writes and restores the volatile
ADDR register. It repeats every five seconds at 115200 baud. Both GDO pins
are inputs. The radio stays in IDLE after reset. GPIO mapping follows
the supplied wiring. Power the CC1101 from 3.3 V.

## Identify and build

Run these from this directory on a Mac with USB serial and package
repository access. The ROM query briefly resets the ESP32 and reads the
flash ID and detected size without writing to flash. Verify the chip is a
classic ESP32 and check its actual flash size against the PlatformIO board
settings before uploading.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install 'esptool>=4.8,<5' platformio
.venv/bin/python -m esptool --port /dev/cu.usbserial-0001 --no-stub flash_id
.venv/bin/pio run
```

## Upload after approval

The normal PlatformIO upload writes the bootloader, partition table, and
application images along with framework support images. It can make the
existing application and its data inaccessible. A full flash backup is
possible on compatible hardware before upload. Substitute the detected
flash capacity in bytes for SIZE_IN_BYTES if taking a backup.

```sh
# Optional backup
.venv/bin/python -m esptool --chip esp32 --port /dev/cu.usbserial-0001 read_flash 0 "SIZE_IN_BYTES" existing-flash.bin
```

```sh
.venv/bin/pio run -t upload --upload-port /dev/cu.usbserial-0001
.venv/bin/pio device monitor --port /dev/cu.usbserial-0001 --baud 115200
```

## Read the results

A pass requires a plausible identity, post-reset IDLE state, stable repeated
reads, and both write/read patterns plus restoration. Consult the actual
PARTNUM and VERSION values in the log. If SO times out, check the common
ground, 3.3 V supply, CSN on GPIO5, and SO on GPIO19. If reads are constant or
the ADDR test fails, check GPIO18 to SCK, GPIO23 to SI, GPIO19 to SO, and CSN
continuity. A successful upload only confirms the ESP32 programming path.

## Capture and integration after SPI verification

Test 303.875/304, 315, and 433 MHz in receive mode. Measure background RSSI
and compare against labeled short presses on each button. The 303.875 and
304 MHz candidates can overlap in a wide receive filter. Refine the estimate
with narrower settings or an independent RF measurement. Verify the module's
matching network and antenna if reception is weak. Record demodulated edge
timings and RSSI with the tuned frequency, bandwidth, and receiver profile.
Try ASK/OOK and 2-FSK profiles separately. Collect multiple independent
presses of each button and compare the frames. Variation alone does not
establish a rolling code. Packet bytes require a matching packet configuration
or offline decoding of the recorded pulses.

For a compatible ASK/OOK protocol, ESPHome's built-in CC1101 component can
use GDO2 on GPIO27 with `remote_receiver` and GDO0 on GPIO4 with
`remote_transmitter`. Use the measured receive profile and decoded commands
to map the supported fan and light functions, with local Wi-Fi and the
native Home Assistant API. Transmitter setup depends on confirming the
frequency, RF profile, and protocol.

### ESPHome receive-only configuration

After the SPI check passes, use `rf-receive.yaml` with an ESPHome release
that includes the built-in CC1101 component. The first-tune frequency,
modulation, bandwidth, and pulse filters are exploratory settings. The
configuration logs signed pulse durations and attempts RC Switch decoding
over USB. It uses GDO2 on GPIO27 and includes no transmit actions. It reports the highest sampled RSSI each second, including during quiet periods.
The current tuning of 433.92 MHz comes from the remote label.

```sh
.venv-esphome/bin/esphome config rf-receive.yaml
.venv-esphome/bin/esphome compile rf-receive.yaml
```

Repeat at 304MHz, 315MHz, 433MHz, and 433.92MHz as appropriate. Each
substitution changes the compiled configuration. Obtain approval before
uploading. At each setting, log at least 10 seconds of background, then
record five separated short presses per button and one sustained press.
Keep a separate log per frequency and label the press order and button.

## Outdoor string lights

The two-button remote is for outdoor string lights. At its labeled
433.92 MHz, the ASK/OOK listener decoded five matching RC Switch protocol 1
frames per button. ON was 0x324F9C1D and OFF was 0x324FA324.
See captures/433920-ook-20261124-214358.log for timestamps and signal levels.
Protocol 1 uses nominal 350 and 1050 microsecond data pulses. The decoder
printed the decoded frames without their individual pulse widths.

Use outdoor-string-lights.yaml for the local Home Assistant light. Put
wifi_ssid, wifi_password, and string_lights_api_key in secrets.yaml beside
the configuration. The API key is a base64-encoded 32-byte random value
and is also used when pairing the ESPHome device with Home Assistant.
Generate a key locally with openssl rand -base64 32.

```sh
.venv-esphome/bin/esphome compile outdoor-string-lights.yaml
# After authorizing this upload
.venv-esphome/bin/esphome upload outdoor-string-lights.yaml --device /dev/cu.usbserial-0001
```

In Home Assistant, open Settings → Devices & services and add the discovered
ESPHome device. If needed, enter string-lights-radio.local or its LAN IP
and the encryption key. The light shows the last locally commanded state.
Five repeats and 0 dBm transmit power are initial settings for a physical
ON/OFF test. The light controller has provided no state feedback.
