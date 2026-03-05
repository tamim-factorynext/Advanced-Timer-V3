#pragma once

#include <stdint.h>

namespace at {
namespace platform {

void initTaskWatchdog(uint32_t timeoutSec);
void addCurrentTaskToWatchdog();
void feedCurrentTaskWatchdog();

}  // namespace platform
}  // namespace at

