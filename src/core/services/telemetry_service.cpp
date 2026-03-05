#include "core/services/telemetry_service.hpp"

namespace at {
namespace services {

TelemetryService::TelemetryService(runtime::LoopStats &core0Stats,
                                   runtime::LoopStats &core1Stats,
                                   portMUX_TYPE &statsMux,
                                   const TaskHandle_t *core0TaskHandle,
                                   const TaskHandle_t *core1TaskHandle,
                                   runtime::StatusSnapshotModel *statusModel,
                                   uint32_t core0PeriodMs,
                                   uint32_t core1PeriodMs,
                                   uint32_t logIntervalMs)
    : core0Stats_(core0Stats),
      core1Stats_(core1Stats),
      statsMux_(statsMux),
      core0TaskHandle_(core0TaskHandle),
      core1TaskHandle_(core1TaskHandle),
      statusModel_(statusModel),
      core0PeriodMs_(core0PeriodMs),
      core1PeriodMs_(core1PeriodMs),
      logIntervalMs_(logIntervalMs) {}

const char *TelemetryService::name() const {
  return "telemetry";
}

void TelemetryService::init() {
  lastLogMs_ = 0;
}

void TelemetryService::tick(uint32_t nowMs) {
  runtime::LoopStats core0Copy;
  runtime::LoopStats core1Copy;
  portENTER_CRITICAL(&statsMux_);
  core0Copy = core0Stats_;
  core1Copy = core1Stats_;
  portEXIT_CRITICAL(&statsMux_);

  const uint32_t core0BudgetUs = core0PeriodMs_ * 1000;
  const uint32_t core1BudgetUs = core1PeriodMs_ * 1000;
  const uint32_t core0AvgUs =
      core0Copy.cycles ? static_cast<uint32_t>(core0Copy.totalBusyUs / core0Copy.cycles) : 0;
  const uint32_t core1AvgUs =
      core1Copy.cycles ? static_cast<uint32_t>(core1Copy.totalBusyUs / core1Copy.cycles) : 0;

  const float core0LoadPct = (core0BudgetUs > 0) ? (100.0f * core0AvgUs / core0BudgetUs) : 0.0f;
  const float core1LoadPct = (core1BudgetUs > 0) ? (100.0f * core1AvgUs / core1BudgetUs) : 0.0f;

  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t minFreeHeap = ESP.getMinFreeHeap();
  const bool hasPsram = psramFound();
  const uint32_t freePsram = hasPsram ? ESP.getFreePsram() : 0;
  const uint32_t minFreePsram = hasPsram ? ESP.getMinFreePsram() : 0;

  const UBaseType_t core0StackWords =
      (core0TaskHandle_ && *core0TaskHandle_) ? uxTaskGetStackHighWaterMark(*core0TaskHandle_) : 0;
  const UBaseType_t core1StackWords =
      (core1TaskHandle_ && *core1TaskHandle_) ? uxTaskGetStackHighWaterMark(*core1TaskHandle_) : 0;

  if (statusModel_ != nullptr) {
    runtime::SystemStatusSnapshot snapshot;
    snapshot.uptimeMs = nowMs;
    snapshot.wifiStatus = WiFi.status();
    snapshot.wifiRssi = (snapshot.wifiStatus == WL_CONNECTED) ? WiFi.RSSI() : 0;
    snapshot.ip = WiFi.localIP();
    snapshot.core0AvgUs = core0AvgUs;
    snapshot.core0MaxUs = core0Copy.maxBusyUs;
    snapshot.core0Overruns = core0Copy.overrunCount;
    snapshot.core0LoadPct = core0LoadPct;
    snapshot.core1AvgUs = core1AvgUs;
    snapshot.core1MaxUs = core1Copy.maxBusyUs;
    snapshot.core1Overruns = core1Copy.overrunCount;
    snapshot.core1LoadPct = core1LoadPct;
    snapshot.heapFree = freeHeap;
    snapshot.heapMin = minFreeHeap;
    snapshot.psramPresent = hasPsram;
    snapshot.psramFree = freePsram;
    snapshot.psramMin = minFreePsram;
    snapshot.core0StackWords = static_cast<uint32_t>(core0StackWords);
    snapshot.core1StackWords = static_cast<uint32_t>(core1StackWords);
    statusModel_->write(snapshot);
  }

  if (nowMs - lastLogMs_ < logIntervalMs_) {
    return;
  }
  logMetricsSnapshot();
  lastLogMs_ = nowMs;
}

void TelemetryService::logMetricsSnapshot() {
  runtime::SystemStatusSnapshot snapshot;
  if (statusModel_ != nullptr) {
    snapshot = statusModel_->read();
  }
  Serial.printf(
      "[METRICS] C0(avg/max/ovr): %lu/%lu/%lu us load=%.1f%% | "
      "C1(avg/max/ovr): %lu/%lu/%lu us load=%.1f%%\n",
      static_cast<unsigned long>(snapshot.core0AvgUs),
      static_cast<unsigned long>(snapshot.core0MaxUs),
      static_cast<unsigned long>(snapshot.core0Overruns),
      snapshot.core0LoadPct,
      static_cast<unsigned long>(snapshot.core1AvgUs),
      static_cast<unsigned long>(snapshot.core1MaxUs),
      static_cast<unsigned long>(snapshot.core1Overruns),
      snapshot.core1LoadPct);

  Serial.printf(
      "[MEM] heap free/min: %lu/%lu | psram present: %s free/min: %lu/%lu | "
      "stackHW words C0/C1: %lu/%lu\n",
      static_cast<unsigned long>(snapshot.heapFree),
      static_cast<unsigned long>(snapshot.heapMin),
      snapshot.psramPresent ? "yes" : "no",
      static_cast<unsigned long>(snapshot.psramFree),
      static_cast<unsigned long>(snapshot.psramMin),
      static_cast<unsigned long>(snapshot.core0StackWords),
      static_cast<unsigned long>(snapshot.core1StackWords));
}

}  // namespace services
}  // namespace at
