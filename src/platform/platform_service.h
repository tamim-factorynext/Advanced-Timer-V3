#pragma once

#include <stdint.h>

namespace v3::platform {

class PlatformService {
 public:
  void begin();
  uint32_t nowMs() const;
  bool initTaskWatchdog(uint32_t timeoutSeconds, bool panicOnTrigger);
  bool addCurrentTaskToWatchdog() const;
  bool resetTaskWatchdog() const;
};

}  // namespace v3::platform
