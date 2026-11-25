#pragma once

#include <cstdint>
#include <vector>

namespace minka {
// Twelve bits, MSB first. The first eight address bits were zero on this remote.
enum Command : uint16_t { STOP = 0x00A, LOW = 0x002, MEDIUM = 0x004, HIGH = 0x008,
                          DIM = 0x005, BRIGHTEN = 0x009 };

inline Command fan_command(bool on, int speed) {
  if (!on) return STOP;
  if (speed == 3) return HIGH;
  if (speed == 2) return MEDIUM;
  return LOW;
}

inline std::vector<int32_t> waveform(uint16_t code, bool hold) {
  const int commands = hold ? 42 : 7;
  constexpr int release_frames = 2;
  std::vector<int32_t> pulses;
  pulses.reserve((commands + release_frames) * 26);
  for (int frame = 0; frame < commands + release_frames; ++frame) {
    const uint16_t bits = frame < commands ? code : 0;
    pulses.push_back(330);
    for (int bit = 11; bit >= 0; --bit) {
      const bool one = (bits >> bit) & 1;
      pulses.push_back(one ? -330 : -670);
      pulses.push_back(one ? 670 : 330);
    }
    pulses.push_back(-11700);
  }
  return pulses;
}
}  // namespace minka
