/*
File: src/platform/hw_profile.cpp
Purpose: Implements active build-time hardware profile materialization.

Responsibilities:
- Resolve compile-time profile flags into typed runtime-readable structures.

Used By:
- src/platform/hw_profile.h
- src/platform/platform_service.cpp
Flow Hook:
- Board/platform integration and connectivity runtime.
*/
#include "platform/hw_profile.h"

#ifndef AT_PROFILE_NAME
#define AT_PROFILE_NAME "esp32doit_gpio_v1"
#endif

#ifndef AT_PLATFORM_VARIANT
#define AT_PLATFORM_VARIANT "ESP32"
#endif

#ifndef AT_DI_GPIO_LIST
#define AT_DI_GPIO_LIST 32,33
#endif

#ifndef AT_DO_GPIO_LIST
#define AT_DO_GPIO_LIST 25,26
#endif

#ifndef AT_AI_GPIO_LIST
#define AT_AI_GPIO_LIST 34,35
#endif

#ifndef AT_SIO_CAPACITY
#define AT_SIO_CAPACITY 2
#endif

#ifndef AT_MATH_CAPACITY
#define AT_MATH_CAPACITY 2
#endif

#ifndef AT_RTC_CAPACITY
#define AT_RTC_CAPACITY 2
#endif

#ifndef AT_DI_BACKEND
#define AT_DI_BACKEND 0
#endif

#ifndef AT_DO_BACKEND
#define AT_DO_BACKEND 0
#endif

#ifndef AT_AI_BACKEND
#define AT_AI_BACKEND 0
#endif

#ifndef AT_RTC_BACKEND
#define AT_RTC_BACKEND 0
#endif

namespace v3::platform {

namespace {

constexpr uint8_t kDiChannels[] = {AT_DI_GPIO_LIST};
constexpr uint8_t kDoChannels[] = {AT_DO_GPIO_LIST};
constexpr uint8_t kAiChannels[] = {AT_AI_GPIO_LIST};

constexpr HardwareProfile kActiveProfile = {
    AT_PROFILE_NAME,
    AT_PLATFORM_VARIANT,
    kDiChannels,
    static_cast<uint8_t>(sizeof(kDiChannels) / sizeof(kDiChannels[0])),
    kDoChannels,
    static_cast<uint8_t>(sizeof(kDoChannels) / sizeof(kDoChannels[0])),
    kAiChannels,
    static_cast<uint8_t>(sizeof(kAiChannels) / sizeof(kAiChannels[0])),
    static_cast<uint8_t>(AT_SIO_CAPACITY),
    static_cast<uint8_t>(AT_MATH_CAPACITY),
    static_cast<uint8_t>(AT_RTC_CAPACITY),
    static_cast<DigitalIoBackend>(AT_DI_BACKEND),
    static_cast<DigitalIoBackend>(AT_DO_BACKEND),
    static_cast<AnalogIoBackend>(AT_AI_BACKEND),
    static_cast<RtcBackend>(AT_RTC_BACKEND),
};

}  // namespace

const HardwareProfile& activeHardwareProfile() { return kActiveProfile; }

}  // namespace v3::platform
