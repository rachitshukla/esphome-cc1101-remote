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
  using Timing = minka::Dimmer<500, 300>;
  minka::LampEstimate lamp;
  assert(!Timing::pending(lamp, false, 0.5f));
  auto toggle = Timing::next(lamp, true, 1.0f);
  assert(toggle.command == minka::BRIGHTEN && toggle.frames == 7);
  assert(!Timing::pending(lamp, true, 1.0f));
  auto dim = Timing::next(lamp, true, 0.5f);
  assert(dim.command == minka::DIM && dim.frames > toggle.frames);
  assert(!Timing::pending(lamp, true, 0.5f));
  auto saved = lamp.brightness;
  assert(Timing::next(lamp, false, 1.0f).frames == 7);
  assert(!Timing::pending(lamp, false, 1.0f) && lamp.brightness == saved);
  assert(Timing::next(lamp, true, 0.5f).frames == 7);
  assert(!Timing::pending(lamp, true, 0.5f));
  assert(Timing::next(lamp, true, 1.0f).command == minka::BRIGHTEN);
  for (int i = 0; i <= 255; ++i) {
    using Slow = minka::Dimmer<3000, 350>;
    float target = i / 255.0f;
    int count = 0;
    while (Slow::pending(lamp, true, target)) {
      auto press = Slow::next(lamp, true, target);
      assert(press.frames > 7 && press.frames <= 42 && ++count < 10);
    }
  }
  using rf_bands::supported;
  for (auto band : {std::pair{300000000u, 348000000u}, std::pair{387000000u, 464000000u},
                    std::pair{779000000u, 928000000u}}) {
    assert(supported(band.first) && supported(band.second));
    assert(!supported(band.first - 1) && !supported(band.second + 1));
  }
  assert(!enabled());
  assert(device.events.empty());
  boot();
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
