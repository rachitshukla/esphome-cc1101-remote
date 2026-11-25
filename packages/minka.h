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

constexpr int TAP_FRAMES = 7;
constexpr int MAX_COMMAND_FRAMES = 42;
constexpr int FRAME_US = 330 + 12 * 1000 + 11700;
constexpr int RELEASE_FRAMES = 2;

struct Press {
  Command command;
  int frames;
};

// The receiver gives no feedback. Keep the last transmitted power and relative
// position independently of the requested slider value, including while off.
struct LampEstimate {
  bool on = false;
  uint8_t brightness = 255;
};

template<int RampMs, int HoldLeadMs> struct Dimmer {
  static constexpr int frames_for_ms(int ms) { return (ms * 1000 + FRAME_US / 2) / FRAME_US; }
  static constexpr int range = frames_for_ms(RampMs);
  static constexpr int lead = frames_for_ms(HoldLeadMs);
  static_assert(range >= 1 && range <= 255, "Ramp must span 1 to 255 frames");
  static_assert(lead >= TAP_FRAMES && lead < MAX_COMMAND_FRAMES, "Invalid hold lead time");

  static int position(float brightness) { return static_cast<int>(brightness * range + 0.5f); }
  static int estimated_position(const LampEstimate &lamp) {
    return (lamp.brightness * range + 127) / 255;
  }
  static bool pending(const LampEstimate &lamp, bool on, float brightness) {
    return lamp.on != on || (on && position(brightness) != estimated_position(lamp));
  }
  // Called once for each pending press. Bound the waveform to the size of the
  // original one-second hold and reconsider the latest target between presses.
  static Press next(LampEstimate &lamp, bool on, float brightness) {
    if (lamp.on != on) {
      lamp.on = on;
      return {BRIGHTEN, TAP_FRAMES};
    }
    const int current = estimated_position(lamp);
    const int delta = position(brightness) - current;
    const bool up = delta > 0;
    int steps = up ? delta : -delta;
    if (steps > MAX_COMMAND_FRAMES - lead) steps = MAX_COMMAND_FRAMES - lead;
    lamp.brightness = ((current + (up ? steps : -steps)) * 255 + range / 2) / range;
    return {up ? BRIGHTEN : DIM, lead + steps};
  }
};

inline std::vector<int32_t> waveform(uint16_t code, int commands) {
  std::vector<int32_t> pulses;
  pulses.reserve((commands + RELEASE_FRAMES) * 26);
  for (int frame = 0; frame < commands + RELEASE_FRAMES; ++frame) {
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
