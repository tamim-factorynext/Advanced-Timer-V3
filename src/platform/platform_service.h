#pragma once

#include <stdint.h>

namespace v3::platform {

class PlatformService {
 public:
  void begin();
  uint32_t nowMs() const;
};

}  // namespace v3::platform
