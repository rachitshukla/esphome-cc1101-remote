#pragma once

#include <cstdint>

namespace rf_bands {

constexpr bool supported(uint32_t hz) {
  return (hz >= 300000000 && hz <= 348000000) ||
         (hz >= 387000000 && hz <= 464000000) ||
         (hz >= 779000000 && hz <= 928000000);
}

}  // namespace rf_bands
