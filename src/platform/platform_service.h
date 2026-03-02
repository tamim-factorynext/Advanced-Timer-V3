#pragma once

#include <stdint.h>

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
  void configureInputPin(uint8_t pin) const;
  void configureOutputPin(uint8_t pin) const;
  bool readDigitalInput(uint8_t pin) const;
  uint32_t readAnalogInput(uint8_t pin) const;
  void writeDigitalOutput(uint8_t pin, bool value) const;
  bool initTaskWatchdog(uint32_t timeoutSeconds, bool panicOnTrigger);
  bool addCurrentTaskToWatchdog() const;
  bool resetTaskWatchdog() const;
};

}  // namespace v3::platform
