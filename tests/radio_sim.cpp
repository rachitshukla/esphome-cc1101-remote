// Host adapter for the actual callbacks from the resolved ESPHome configuration.
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "rf_bands.h"
#include "minka.h"

enum Event { frequency, power, transmit, idle };
struct TestRadio {
  std::vector<Event> events;
  uint32_t tuned = expected_hz;

  void set_frequency(uint32_t hz) { events.push_back(frequency); tuned = hz; }
  void set_output_power(float dbm) { assert(dbm == 0); events.push_back(power); }
};
TestRadio device;
TestRadio *radio = &device;
int held_command = 0;
struct HoldScript {
  int starts = 0;
  void execute(int command) { ++starts; held_command = command; }
} hold_minka_light;
struct StopHoldScript {
  int stops = 0;
  void execute() { ++stops; held_command = 0; }
} stop_minka_hold;
#define id(name) name
#include "callbacks.inc"

int main() {
  // Independently decoded fields from five labeled presses per physical button.
  assert(minka::fan_command(false, 3) == 0x00A);
  assert(minka::fan_command(true, 1) == 0x002);
  assert(minka::fan_command(true, 2) == 0x004);
  assert(minka::fan_command(true, 3) == 0x008);
  assert(minka::DIM == 0x005 && minka::BRIGHTEN == 0x009);
  for (auto command : {0x00A, 0x002, 0x004, 0x008, 0x005, 0x009}) {
    for (bool hold : {false, true}) {
      const unsigned repeats = hold ? 42 : 7;
      auto pulses = minka::waveform(command, repeats);
      assert(pulses.size() == (repeats + 2) * 26);
      for (unsigned i = 0; i < repeats + 2; ++i) {
        const auto *frame = pulses.data() + 26 * i;
        assert(frame[0] == 330 && frame[25] == -11700);
        unsigned decoded = 0;
        for (int j = 1; j < 25; j += 2) {
          assert(frame[j + 1] - frame[j] == 1000);
          assert(frame[j] == -330 || frame[j] == -670);
          decoded = (decoded << 1) | (frame[j] == -330);
        }
        assert(decoded == (i < repeats ? command : 0));
      }
    }
  }
  // The captured hold repeats the same command until the two zero release frames.
  for (auto command : {minka::BRIGHTEN, minka::DIM}) {
    auto hold = minka::waveform(command, 21, false);
    auto continuation = minka::waveform(command, 4, false);
    auto release = minka::waveform(0, 0, true);
    assert(hold.size() == 21 * 26 && continuation.size() == 4 * 26);
    assert(release.size() == 2 * 26);
    hold.insert(hold.end(), continuation.begin(), continuation.end());
    hold.insert(hold.end(), release.begin(), release.end());
    assert(hold == minka::waveform(command, 25));
  }
  using rf_bands::supported;
  for (auto band : {std::pair{300000000u, 348000000u}, std::pair{387000000u, 464000000u},
                    std::pair{779000000u, 928000000u}}) {
    assert(supported(band.first) && supported(band.second));
    assert(!supported(band.first - 1) && !supported(band.second + 1));
  }
  assert(!enabled());
  assert(device.events.empty());
  dashboard_hold(true, true);
  assert(held_command == 0 && hold_minka_light.starts == 0);
  boot();
  dashboard_hold(false, true);
  dashboard_hold(false, true);
  dashboard_hold(true, false);
  assert(held_command == minka::DIM && hold_minka_light.starts == 1);
  dashboard_hold(false, false);
  dashboard_hold(false, false);
  assert(held_command == 0 && stop_minka_hold.stops == 1);
  assert(enabled() && device.events == std::vector<Event>{idle});
  assert(lights_frequency() == expected_hz);
  assert(code(true) == expected_on && code(false) == expected_off);

  for (uint32_t hz : {expected_hz, expected_hz, 303875000u, expected_hz}) {
    const bool changing = hz != device.tuned;
    device.events.clear();
    next_tx_hz = hz;
    start_frame();
    assert(device.tuned == hz);
    finish_frame();
    const std::vector<Event> expected = changing
        ? std::vector<Event>{frequency, power, transmit, idle}
        : std::vector<Event>{transmit, idle};
    assert(device.events == expected);
  }
}
