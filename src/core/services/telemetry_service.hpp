#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "core/contracts/core_service.hpp"
#include "core/runtime/runtime_types.hpp"
#include "core/runtime/status_snapshot.hpp"

namespace at {
namespace services {

class TelemetryService : public core::CoreService {
 public:
  TelemetryService(runtime::LoopStats &core0Stats,
                   runtime::LoopStats &core1Stats,
                   portMUX_TYPE &statsMux,
                   const TaskHandle_t *core0TaskHandle,
                   const TaskHandle_t *core1TaskHandle,
                   runtime::StatusSnapshotModel *statusModel,
                   uint32_t core0PeriodMs,
                   uint32_t core1PeriodMs,
                   uint32_t logIntervalMs);

  const char *name() const override;
  void init() override;
  void tick(uint32_t nowMs) override;

 private:
  void logMetricsSnapshot();

  runtime::LoopStats &core0Stats_;
  runtime::LoopStats &core1Stats_;
  portMUX_TYPE &statsMux_;
  const TaskHandle_t *core0TaskHandle_;
  const TaskHandle_t *core1TaskHandle_;
  runtime::StatusSnapshotModel *statusModel_;
  uint32_t core0PeriodMs_;
  uint32_t core1PeriodMs_;
  uint32_t logIntervalMs_;
  uint32_t lastLogMs_ = 0;
};

}  // namespace services
}  // namespace at
