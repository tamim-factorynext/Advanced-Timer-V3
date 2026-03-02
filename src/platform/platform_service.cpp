#include "platform/platform_service.h"

#include <Arduino.h>

namespace v3::platform {

void PlatformService::begin() {}

uint32_t PlatformService::nowMs() const { return millis(); }

}  // namespace v3::platform
