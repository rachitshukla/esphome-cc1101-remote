// Host adapter for the actual callbacks from the resolved ESPHome configuration.
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "rf_bands.h"

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

  for (uint32_t hz : {expected_hz, expected_hz, 315000000u, expected_hz}) {
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
