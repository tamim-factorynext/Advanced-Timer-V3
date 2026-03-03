/*
File: src/storage/v3_config_contract.h
Purpose: Declares the v3 config contract module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/platform/wifi_runtime.h
- src/storage/v3_config_decoder.h
- src/storage/v3_config_validator.h
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

namespace v3::storage {

constexpr uint32_t kConfigSchemaVersion = 300;
constexpr uint32_t kMinScanIntervalMs = 10;
constexpr uint32_t kMaxScanIntervalMs = 1000;
constexpr uint8_t kMaxCards = 64;
constexpr uint8_t kMaxWiFiSsidLen = 32;
constexpr uint8_t kMaxWiFiPasswordLen = 64;
constexpr uint8_t kMaxNtpServerLen = 63;
constexpr uint8_t kMaxTimezoneLen = 47;

/**
 * @brief Enumerates fixed card families supported by V3 config model.
 * @details Determines which parameter block in `CardConfig` is meaningful.
 * @par Used By
 * - src/storage/v3_config_decoder.cpp
 * - src/storage/v3_config_validator.cpp
 * - src/kernel/kernel_service.cpp
 */
enum class CardFamily : uint8_t { DI, DO, AI, SIO, MATH, RTC };

/**
 * @brief Supported operators for condition-clause evaluation.
 * @details Operators map to runtime signal semantics across logic/value/mission domains.
 * @par Used By
 * - src/storage/v3_config_decoder.cpp
 * - src/storage/v3_config_validator.cpp
 * - src/kernel/kernel_service.cpp
 */
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

/**
 * @brief Combines two condition clauses into one condition block result.
 * @details `None` means clauseA-only semantics.
 * @par Used By
 * - src/storage/v3_config_decoder.cpp
 * - src/kernel/kernel_service.cpp
 */
enum class ConditionCombiner : uint8_t { None, And, Or };

/**
 * @brief One source/operator/threshold condition clause.
 * @details Evaluated against one runtime signal selected by `sourceCardId`.
 * @par Used By
 * - src/storage/v3_config_decoder.cpp
 * - src/kernel/kernel_service.cpp
 */
struct ConditionClause {
  uint8_t sourceCardId;
  ConditionOperator op;
  uint32_t threshold;
};

/**
 * @brief Two-clause condition with combiner policy.
 * @details Forms reusable turn-on/turn-off condition contract for multiple families.
 * @par Used By
 * - src/storage/v3_config_decoder.cpp
 * - src/storage/v3_config_validator.cpp
 * - src/kernel/kernel_service.cpp
 */
struct ConditionBlock {
  ConditionClause clauseA;
  ConditionClause clauseB;
  ConditionCombiner combiner;
};

/** @brief DI-specific config parameters and conditions. */
struct DiParams {
  uint8_t channel;
  bool invert;
  uint32_t debounceMs;
  uint8_t edgeMode;
  bool setEnabled;
  bool resetEnabled;
  ConditionBlock turnOnCondition;
  ConditionBlock turnOffCondition;
};

/** @brief DO-specific config parameters and conditions. */
struct DoParams {
  uint8_t channel;
  bool invert;
  uint8_t mode;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
  ConditionBlock turnOnCondition;
  ConditionBlock turnOffCondition;
};

/** @brief AI-specific scaling and smoothing config parameters. */
struct AiParams {
  uint8_t channel;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t smoothingFactorPct;
};

/** @brief SIO-specific config parameters and conditions. */
struct SioParams {
  bool invert;
  uint8_t mode;
  uint32_t delayBeforeOnMs;
  uint32_t activeDurationMs;
  uint32_t repeatCount;
  ConditionBlock turnOnCondition;
  ConditionBlock turnOffCondition;
};

/** @brief MATH-specific operation, scaling, and condition parameters. */
struct MathParams {
  uint8_t operation;
  uint32_t inputA;
  uint32_t inputB;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t smoothingFactorPct;
  uint32_t fallbackValue;
  ConditionBlock turnOnCondition;
  ConditionBlock turnOffCondition;
};

/** @brief RTC schedule/alarm parameter contract. */
struct RtcParams {
  bool hasYear;
  uint16_t year;
  bool hasMonth;
  uint8_t month;
  bool hasDay;
  uint8_t day;
  bool hasWeekday;
  uint8_t weekday;
  bool hasHour;
  uint8_t hour;
  uint8_t minute;
  uint32_t triggerDurationMs;
};

/** @brief Time-sync server and staleness policy config. */
struct NtpConfig {
  bool enabled;
  char primaryTimeServer[kMaxNtpServerLen + 1];
  char secondaryServer[kMaxNtpServerLen + 1];
  char tertiaryServer[kMaxNtpServerLen + 1];
  uint32_t syncIntervalSec;
  uint32_t startupTimeoutSec;
  uint32_t maxTimeAgeSec;
};

/** @brief Global time configuration container. */
struct ClockConfig {
  char timezone[kMaxTimezoneLen + 1];
  NtpConfig timeSync;
};

/** @brief One WiFi credential policy slot. */
struct WiFiCredential {
  char ssid[kMaxWiFiSsidLen + 1];
  char password[kMaxWiFiPasswordLen + 1];
  uint32_t timeoutSec;
  bool editable;
};

/** @brief Global WiFi behavior and credential policy config. */
struct WiFiConfig {
  WiFiCredential backupAccessNetwork;
  WiFiCredential userConfiguredNetwork;
  uint32_t retryDelaySec;
  bool staOnly;
};

/** @brief One configured card row including family-specific parameter blocks. */
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

/**
 * @brief Top-level V3 system configuration contract.
 * @details Single source used by decode/validate/bootstrap and kernel begin.
 * @par Used By
 * - src/storage/storage_service.cpp
 * - src/kernel/kernel_service.cpp
 * - src/platform/wifi_runtime.cpp
 */
struct SystemConfig {
  uint32_t schemaVersion;
  uint32_t scanPeriodMs;
  WiFiConfig wifi;
  ClockConfig time;
  uint8_t cardCount;
  CardConfig cards[kMaxCards];
};

/**
 * @brief Builds default system config contract values.
 * @details Provides deterministic fallback baseline when no valid file config is available.
 * @return Default-initialized system config.
 * @par Used By
 * - src/storage/storage_service.cpp
 */
SystemConfig makeDefaultSystemConfig();

}  // namespace v3::storage



