/*
File: src/kernel/legacy_v3_card_types.h
Purpose: Declares the v3 card types module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/legacy_v3_card_bridge.h
- src/kernel/legacy_v3_runtime_store.h
- src/kernel/legacy_v3_typed_card_parser.h
- (+ more call sites)
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

/** @brief Fixed V3 card-family identity used by typed config/runtime paths. */
enum class V3CardFamily : uint8_t { DI, DO, AI, SIO, MATH, RTC };

/** @brief Fault severity policy associated with typed card configuration. */
enum class V3FaultPolicy : uint8_t { INFO, WARN, CRITICAL };

/** @brief Declares supported signal channels for a family. */
struct V3SignalSupport {
  bool hasLogicalState;
  bool hasPhysicalState;
  bool hasCurrentValue;
  bool hasTriggerFlag;
  bool hasMissionState;
};

/**
 * @brief Returns signal-support profile for given V3 card family.
 * @details Used to constrain condition/source capabilities across families.
 * @par Used By
 * - src/kernel/legacy_v3_typed_card_parser.cpp
 */
inline V3SignalSupport signalSupportForFamily(V3CardFamily family) {
  switch (family) {
    case V3CardFamily::DI:
      return {true, true, true, true, false};
    case V3CardFamily::DO:
      return {true, true, true, true, true};
    case V3CardFamily::AI:
      return {false, false, true, false, false};
    case V3CardFamily::SIO:
      return {true, true, true, true, true};
    case V3CardFamily::MATH:
      return {false, false, true, true, false};
    case V3CardFamily::RTC:
      return {true, false, false, true, false};
    default:
      return {false, false, false, false, false};
  }
}

/** @brief Typed condition block contract used by typed card families. */
struct V3ConditionBlock {
  uint8_t clauseAId;
  logicOperator clauseAOperator;
  uint32_t clauseAThreshold;
  uint8_t clauseAThresholdCardId;
  bool clauseAUseThresholdCard;
  uint8_t clauseBId;
  logicOperator clauseBOperator;
  uint32_t clauseBThreshold;
  uint8_t clauseBThresholdCardId;
  bool clauseBUseThresholdCard;
  combineMode combiner;
};

/** @brief Typed DI configuration block. */
struct V3DiConfig {
  uint8_t channel;
  bool invert;
  uint32_t debounceTimeMs;
  cardMode edgeMode;
  V3ConditionBlock turnOnCondition;
  V3ConditionBlock turnOffCondition;
};

/** @brief Typed DO configuration block. */
struct V3DoConfig {
  uint8_t channel;
  bool invert;
  cardMode mode;
  uint32_t delayBeforeOnMs;
  uint32_t onDurationMs;
  uint32_t repeatCount;
  V3ConditionBlock turnOnCondition;
  V3ConditionBlock turnOffCondition;
};

/** @brief Typed AI configuration block. */
struct V3AiConfig {
  uint8_t channel;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t smoothingFactorPct;
};

/** @brief Typed SIO configuration block. */
struct V3SioConfig {
  bool invert;
  cardMode mode;
  uint32_t delayBeforeOnMs;
  uint32_t onDurationMs;
  uint32_t repeatCount;
  V3ConditionBlock turnOnCondition;
  V3ConditionBlock turnOffCondition;
};

/** @brief Typed MATH configuration block. */
struct V3MathConfig {
  uint8_t operation;
  uint32_t fallbackValue;
  uint32_t inputA;
  uint32_t inputB;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t smoothingFactorPct;
  V3ConditionBlock turnOnCondition;
  V3ConditionBlock turnOffCondition;
};

/** @brief Typed RTC schedule configuration block. */
struct V3RtcConfig {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
  bool hasYear;
  bool hasMonth;
  bool hasDay;
  bool hasWeekday;
  bool hasHour;
  uint32_t triggerDurationMs;
};

/**
 * @brief Typed per-card configuration envelope.
 * @details One row contains shared card metadata and family-specific parameter blocks.
 * @par Used By
 * - src/kernel/legacy_v3_typed_card_parser.cpp
 * - src/kernel/legacy_v3_runtime_store.cpp
 * - src/kernel/legacy_v3_card_bridge.cpp
 */
struct V3CardConfig {
  uint8_t cardId;
  V3CardFamily family;
  bool enabled;
  V3FaultPolicy faultPolicy;

  V3DiConfig di;
  V3DoConfig dout;
  V3AiConfig ai;
  V3SioConfig sio;
  V3MathConfig math;
  V3RtcConfig rtc;
};



