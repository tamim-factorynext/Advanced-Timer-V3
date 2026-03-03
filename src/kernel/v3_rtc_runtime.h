#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

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

struct V3RtcMinuteStamp {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
};

struct V3RtcRuntimeConfig {
  uint32_t triggerDurationMs;
};

struct V3RtcRuntimeState {
  bool commandState;
  bool actualState;
  bool edgePulse;
  uint32_t liveValue;
  uint32_t triggerStartMs;
  cardMode mode;
  cardState state;
};

struct V3RtcStepInput {
  uint32_t nowMs;
};

void runV3RtcStep(const V3RtcRuntimeConfig& cfg, V3RtcRuntimeState& runtime,
                  const V3RtcStepInput& in);

bool v3RtcFieldMatches(int fieldValue, int scheduleValue);
bool v3RtcChannelMatchesMinute(const V3RtcScheduleView& channel,
                               const V3RtcMinuteStamp& stamp);
uint32_t v3RtcMinuteKey(const V3RtcMinuteStamp& stamp);

