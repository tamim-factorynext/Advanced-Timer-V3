#pragma once

#include <stdint.h>

namespace v3::control {

class ControlService {
 public:
  void begin();
  void tick(uint32_t nowMs);

 private:
  uint32_t lastTickMs_ = 0;
};

}  // namespace v3::control
