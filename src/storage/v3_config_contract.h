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

enum class ConditionOperator : uint8_t {
  AlwaysTrue,
  AlwaysFalse,
  LogicalTrue,
  LogicalFalse,
  PhysicalOn,
  PhysicalOff,
  Triggered,
  TriggerCleared,
  GT,
  LT,
  EQ,
  NEQ,
  GTE,
  LTE,
  Running,
  Finished,
  Stopped,
};

enum class ConditionCombiner : uint8_t { None, And, Or };

struct ConditionClause {
  uint8_t sourceCardId;
  ConditionOperator op;
  uint32_t threshold;
};

struct ConditionBlock {
  ConditionClause clauseA;
  ConditionClause clauseB;
  ConditionCombiner combiner;
};

struct DiParams {
  uint8_t channel;
  bool invert;
  uint32_t debounceMs;
  uint8_t edgeMode;
  bool setEnabled;
  bool resetEnabled;
  ConditionBlock setCondition;
  ConditionBlock resetCondition;
};

struct DoParams {
  uint8_t channel;
  bool invert;
  uint8_t mode;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
  ConditionBlock setCondition;
  ConditionBlock resetCondition;
};

struct AiParams {
  uint8_t channel;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t emaAlphaX100;
};

struct SioParams {
  bool invert;
  uint8_t mode;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
  ConditionBlock setCondition;
  ConditionBlock resetCondition;
};

struct MathParams {
  uint8_t operation;
  uint32_t inputA;
  uint32_t inputB;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t emaAlphaX100;
  uint32_t fallbackValue;
  ConditionBlock setCondition;
  ConditionBlock resetCondition;
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
