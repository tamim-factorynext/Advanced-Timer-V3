#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3RtcScheduleView {
  bool enabled;
  int16_t year;     // -1 wildcard
  int8_t month;     // -1 wildcard
  int8_t day;       // -1 wildcard
  int8_t weekday;   // -1 wildcard
  int8_t hour;      // -1 wildcard
  int8_t minute;    // -1 wildcard
  uint8_t rtcCardId;
};

struct V3RtcMinuteStamp {
  int year;
  int month;
  int day;
  int weekday;
  int hour;
  int minute;
};

struct V3RtcRuntimeConfig {
  uint32_t triggerDurationMs;
};

struct V3RtcRuntimeState {
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
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
int32_t v3RtcMinuteKey(const V3RtcMinuteStamp& stamp);
