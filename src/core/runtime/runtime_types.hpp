#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace at {
namespace runtime {

struct LoopStats {
  uint64_t cycles = 0;
  uint64_t totalBusyUs = 0;
  uint32_t maxBusyUs = 0;
  uint32_t overrunCount = 0;
};

inline void updateLoopStats(LoopStats &stats,
                            uint32_t busyUs,
                            uint32_t budgetUs,
                            portMUX_TYPE &mux) {
  portENTER_CRITICAL(&mux);
  stats.cycles++;
  stats.totalBusyUs += busyUs;
  if (busyUs > stats.maxBusyUs) {
    stats.maxBusyUs = busyUs;
  }
  if (busyUs > budgetUs) {
    stats.overrunCount++;
  }
  portEXIT_CRITICAL(&mux);
}

}  // namespace runtime
}  // namespace at

