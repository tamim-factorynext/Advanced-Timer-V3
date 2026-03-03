/*
File: src/kernel/v3_rtc_runtime.cpp
Purpose: Implements the v3 rtc runtime module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/kernel_service.h
- src/kernel/v3_rtc_runtime.h
- src/kernel/v3_runtime_adapters.h
- (+ more call sites)
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_rtc_runtime.h"

/**
 * @brief Executes one RTC runtime step.
 * @details Maintains active trigger window and clears command state when duration expires.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
void runV3RtcStep(const V3RtcRuntimeConfig& cfg, V3RtcRuntimeState& runtime,
                  const V3RtcStepInput& in) {
  runtime.mode = Mode_None;
  runtime.state = State_None;
  runtime.actualState = false;
  runtime.liveValue = 0;
  runtime.edgePulse = false;

  if (!runtime.commandState) {
    return;
  }

  if (cfg.triggerDurationMs > 0 &&
      (in.nowMs - runtime.triggerStartMs) >= cfg.triggerDurationMs) {
    runtime.commandState = false;
    return;
  }
}

/** @brief Compares one time field against configured schedule value. */
bool v3RtcFieldMatches(int fieldValue, int scheduleValue) {
  return fieldValue == scheduleValue;
}

/**
 * @brief Evaluates minute-stamp match against one RTC schedule view.
 * @details Applies enabled-field matching on year/month/day/weekday/hour plus required minute.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
bool v3RtcChannelMatchesMinute(const V3RtcScheduleView& channel,
                               const V3RtcMinuteStamp& stamp) {
  if (!channel.enabled) return false;
  if (stamp.minute != channel.minute) return false;
  if (channel.hasHour && stamp.hour != channel.hour) return false;
  if (channel.hasWeekday && stamp.weekday != channel.weekday) return false;
  if (channel.hasDay && stamp.day != channel.day) return false;
  if (channel.hasMonth && stamp.month != channel.month) return false;
  if (channel.hasYear && stamp.year != channel.year) return false;
  return true;
}

/**
 * @brief Builds monotonic minute key for deduplication within one minute bucket.
 * @details Used to prevent repeated trigger activation for same channel+minute.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
uint32_t v3RtcMinuteKey(const V3RtcMinuteStamp& stamp) {
  return ((((static_cast<uint32_t>(stamp.year) * 13U) + stamp.month) * 32U +
           stamp.day) *
              24U +
          stamp.hour) *
             60U +
         stamp.minute;
}

