#pragma once

#include <cstdint>
#include "esphome/components/cc1101/cc1101.h"

namespace rf_measurement {

// The CC1101 uses signed half-dB units. 74 dB is the nominal RSSI offset.
inline float read_dbm(esphome::cc1101::CC1101Component &radio) {
  radio.enable();
  radio.transfer_byte(0xF4);  // RSSI status register with the burst-read bit.
  const int raw = radio.transfer_byte(0);
  radio.disable();
  const int signed_value = raw < 128 ? raw : raw - 256;
  return 0.5f * signed_value - 74.0f;
}

}  // namespace rf_measurement
