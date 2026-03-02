#pragma once

#include <stdint.h>

namespace v3::storage {

constexpr uint32_t kConfigSchemaVersion = 300;
constexpr uint32_t kMinScanIntervalMs = 10;
constexpr uint32_t kMaxScanIntervalMs = 1000;
constexpr uint8_t kMaxCards = 64;
constexpr uint8_t kMaxWiFiSsidLen = 32;
constexpr uint8_t kMaxWiFiPasswordLen = 64;

enum class CardFamily : uint8_t { DI, DO, AI, SIO, MATH, RTC };

struct DiParams {
  bool invert;
  uint32_t debounceMs;
};

struct DoParams {
  uint32_t onDelayMs;
  uint32_t onDurationMs;
  uint32_t repeatCount;
};

struct AiParams {
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t emaAlphaX100;
};

struct SioParams {
  uint32_t onDelayMs;
  uint32_t onDurationMs;
  uint32_t repeatCount;
};

struct MathParams {
  uint32_t thresholdCentiunits;
  uint32_t clampMin;
  uint32_t clampMax;
};

struct RtcParams {
  uint8_t hour;
  uint8_t minute;
  uint16_t durationSeconds;
};

struct WiFiCredential {
  char ssid[kMaxWiFiSsidLen + 1];
  char password[kMaxWiFiPasswordLen + 1];
  uint32_t timeoutSec;
  bool editable;
};

struct WiFiConfig {
  WiFiCredential master;
  WiFiCredential user;
  uint32_t retryBackoffSec;
  bool staOnly;
};

struct CardConfig {
  uint8_t id;
  CardFamily family;
  bool enabled;
  DiParams di;
  DoParams dout;
  AiParams ai;
  SioParams sio;
  MathParams math;
  RtcParams rtc;
};

struct SystemConfig {
  uint32_t schemaVersion;
  uint32_t scanIntervalMs;
  WiFiConfig wifi;
  uint8_t cardCount;
  CardConfig cards[kMaxCards];
};

SystemConfig makeDefaultSystemConfig();

}  // namespace v3::storage
