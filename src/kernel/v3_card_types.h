#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

enum class V3CardFamily : uint8_t { DI, DO, AI, SIO, MATH, RTC };

enum class V3FaultPolicy : uint8_t { INFO, WARN, CRITICAL };

struct V3SignalSupport {
  bool hasLogicalState;
  bool hasPhysicalState;
  bool hasCurrentValue;
  bool hasTriggerFlag;
  bool hasMissionState;
};

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
      return {false, false, true, false, false};
    case V3CardFamily::RTC:
      return {true, true, true, true, false};
    default:
      return {false, false, false, false, false};
  }
}

struct V3ConditionBlock {
  uint8_t clauseAId;
  logicOperator clauseAOperator;
  uint32_t clauseAThreshold;
  uint8_t clauseBId;
  logicOperator clauseBOperator;
  uint32_t clauseBThreshold;
  combineMode combiner;
};

struct V3DiConfig {
  uint8_t channel;
  bool invert;
  uint32_t debounceTimeMs;
  cardMode edgeMode;
  V3ConditionBlock set;
  V3ConditionBlock reset;
};

struct V3DoConfig {
  uint8_t channel;
  cardMode mode;
  uint32_t delayBeforeOnMs;
  uint32_t onDurationMs;
  uint32_t repeatCount;
  V3ConditionBlock set;
  V3ConditionBlock reset;
};

struct V3AiConfig {
  uint8_t channel;
  uint32_t inputMin;
  uint32_t inputMax;
  uint32_t outputMin;
  uint32_t outputMax;
  uint32_t emaAlphaX100;
};

struct V3SioConfig {
  cardMode mode;
  uint32_t delayBeforeOnMs;
  uint32_t onDurationMs;
  uint32_t repeatCount;
  V3ConditionBlock set;
  V3ConditionBlock reset;
};

struct V3MathConfig {
  uint32_t fallbackValue;
  uint32_t inputA;
  uint32_t inputB;
  uint32_t clampMin;
  uint32_t clampMax;
  V3ConditionBlock set;
  V3ConditionBlock reset;
};

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
  uint32_t triggerDurationMs;
};

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
