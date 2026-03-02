#include "control/control_service.h"

namespace v3::control {

void ControlService::begin() { lastTickMs_ = 0; }

void ControlService::tick(uint32_t nowMs) { lastTickMs_ = nowMs; }

}  // namespace v3::control
