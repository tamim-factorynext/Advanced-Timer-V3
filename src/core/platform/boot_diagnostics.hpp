#pragma once

#include <stddef.h>
#include <esp_system.h>

namespace at {
namespace platform {

const char *resetReasonToString(esp_reset_reason_t reason);
void runPsramBootProbe(bool expectPsram, size_t probeBytes);

}  // namespace platform
}  // namespace at

