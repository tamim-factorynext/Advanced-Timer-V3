/*
File: src/kernel/legacy_v3_rtc_runtime.h
Purpose: Declares the v3 rtc runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/kernel/legacy_v3_rtc_runtime.cpp
- src/kernel/legacy_v3_runtime_adapters.h
- (+ more call sites)
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

/** @brief RTC schedule matcher view consumed by runtime evaluator. */
struct V3RtcScheduleView {
  bool enabled;
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
  uint8_t rtcCardId;
};

/** @brief One minute-resolution wall-clock stamp for schedule matching. */
struct V3RtcMinuteStamp {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
};

/** @brief RTC runtime configuration contract for one scheduler card. */
struct V3RtcRuntimeConfig {
  uint32_t triggerDurationMs;
};

/** @brief Mutable RTC runtime state projected into snapshots/signals. */
struct V3RtcRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  uint32_t triggerStartMs;
  cardMode mode;
  cardState state;
};

/** @brief RTC step input bundle for one scan pass. */
struct V3RtcStepInput {
  uint32_t nowMs;
};

/**
 * @brief Executes one RTC runtime step.
 * @details Maintains trigger-window output state after schedule-match activation.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
void runV3RtcStep(const V3RtcRuntimeConfig& cfg, V3RtcRuntimeState& runtime,
                  const V3RtcStepInput& in);

/**
 * @brief Checks one schedule field with wildcard-friendly matching.
 * @details Helper for minute-level schedule comparison logic.
 * @par Used By
 * - src/kernel/legacy_v3_rtc_runtime.cpp
 */
bool v3RtcFieldMatches(int fieldValue, int scheduleValue);
/**
 * @brief Evaluates whether schedule view matches provided minute stamp.
 * @details Applies all enabled schedule fields (`year/month/day/weekday/hour/minute`).
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
bool v3RtcChannelMatchesMinute(const V3RtcScheduleView& channel,
                               const V3RtcMinuteStamp& stamp);
/**
 * @brief Builds stable integer key for minute-level deduplication.
 * @details Used to avoid repeated trigger activation within same minute bucket.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
uint32_t v3RtcMinuteKey(const V3RtcMinuteStamp& stamp);



