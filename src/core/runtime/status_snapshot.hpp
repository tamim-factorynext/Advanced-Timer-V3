#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <stdint.h>

namespace at {
namespace runtime {

struct SystemStatusSnapshot {
  uint32_t uptimeMs = 0;
  wl_status_t wifiStatus = WL_DISCONNECTED;
  int32_t wifiRssi = 0;
  uint32_t ip = 0;

  uint32_t core0AvgUs = 0;
  uint32_t core0MaxUs = 0;
  uint32_t core0Overruns = 0;
  float core0LoadPct = 0.0f;

  uint32_t core1AvgUs = 0;
  uint32_t core1MaxUs = 0;
  uint32_t core1Overruns = 0;
  float core1LoadPct = 0.0f;

  uint32_t heapFree = 0;
  uint32_t heapMin = 0;
  bool psramPresent = false;
  uint32_t psramFree = 0;
  uint32_t psramMin = 0;
  uint32_t core0StackWords = 0;
  uint32_t core1StackWords = 0;
};

class StatusSnapshotModel {
 public:
  void write(const SystemStatusSnapshot &snapshot) {
    portENTER_CRITICAL(&mux_);
    snapshot_ = snapshot;
    portEXIT_CRITICAL(&mux_);
  }

  SystemStatusSnapshot read() const {
    SystemStatusSnapshot copy;
    portENTER_CRITICAL(&mux_);
    copy = snapshot_;
    portEXIT_CRITICAL(&mux_);
    return copy;
  }

 private:
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  SystemStatusSnapshot snapshot_;
};

}  // namespace runtime
}  // namespace at

