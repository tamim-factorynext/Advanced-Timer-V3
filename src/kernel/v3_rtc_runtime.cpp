#include "kernel/v3_rtc_runtime.h"

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

bool v3RtcFieldMatches(int fieldValue, int scheduleValue) {
  return fieldValue == scheduleValue;
}

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

uint32_t v3RtcMinuteKey(const V3RtcMinuteStamp& stamp) {
  return ((((static_cast<uint32_t>(stamp.year) * 13U) + stamp.month) * 32U +
           stamp.day) *
              24U +
          stamp.hour) *
             60U +
         stamp.minute;
}

