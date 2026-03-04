/*
File: src/platform/platform_service.h
Purpose: Declares the platform service module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/main.cpp
- src/kernel/kernel_service.h
Flow Hook:
- Board/platform integration and connectivity runtime.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "platform/hw_profile.h"

namespace v3::platform {

struct LocalMinuteStamp {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
};

class PlatformService {
 public:
  void begin();
  uint32_t nowMs() const;
  bool readLocalMinuteStamp(LocalMinuteStamp& out) const;
  void configureDiChannel(uint8_t channel) const;
  void configureAiChannel(uint8_t channel) const;
  void configureDoChannel(uint8_t channel) const;
  bool readDiChannel(uint8_t channel) const;
  uint32_t readAiChannel(uint8_t channel) const;
  void writeDoChannel(uint8_t channel, bool value) const;
  const HardwareProfile& profile() const;
  bool initTaskWatchdog(uint32_t timeoutSeconds, bool panicOnTrigger);
  bool addCurrentTaskToWatchdog() const;
  bool resetTaskWatchdog() const;

 private:
  const HardwareProfile* profile_ = nullptr;
};

}  // namespace v3::platform
