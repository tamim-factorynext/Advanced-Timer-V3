/*
File: src/platform/hw_profile.h
Purpose: Declares build-time hardware profile contracts and active profile access.

Responsibilities:
- Materialize board profile from compile-time flags.
- Keep logical-channel to backend-channel mapping centralized.

Used By:
- src/platform/platform_service.cpp
Flow Hook:
- Board/platform integration and connectivity runtime.
*/
#pragma once

#include <stdint.h>

namespace v3::platform {

enum class DigitalIoBackend : uint8_t { Gpio = 0, I2cExpander = 1, Plugin = 2 };
enum class AnalogIoBackend : uint8_t { InternalAdc = 0, I2cAdc = 1, Plugin = 2 };
enum class RtcBackend : uint8_t {
  RtcMillis = 0,
  Ds3231 = 1,
  Pcf8523 = 2,
  Ds1307 = 3
};

struct HardwareProfile {
  const char* profileName;
  const char* platformVariant;
  const uint8_t* diChannels;
  uint8_t diCount;
  const uint8_t* doChannels;
  uint8_t doCount;
  const uint8_t* aiChannels;
  uint8_t aiCount;
  uint8_t sioCapacity;
  uint8_t mathCapacity;
  uint8_t rtcCapacity;
  DigitalIoBackend diBackend;
  DigitalIoBackend doBackend;
  AnalogIoBackend aiBackend;
  RtcBackend rtcBackend;
};

const HardwareProfile& activeHardwareProfile();

}  // namespace v3::platform
