#pragma once

#include <stdint.h>

#include "runtime/runtime_service.h"

namespace v3::portal {

class PortalService {
 public:
  void begin();
  void tick(uint32_t nowMs, const v3::runtime::RuntimeSnapshot& snapshot);

 private:
  uint32_t lastTickMs_ = 0;
  uint32_t observedScanCount_ = 0;
};

}  // namespace v3::portal
