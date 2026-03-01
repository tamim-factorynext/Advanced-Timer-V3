#include "kernel/v3_rtc_runtime.h"

void runV3RtcStep(const V3RtcRuntimeConfig& cfg, V3RtcRuntimeState& runtime,
                  const V3RtcStepInput& in) {
  runtime.mode = Mode_None;
  runtime.state = State_None;
  runtime.physicalState = runtime.logicalState;

  if (!runtime.logicalState) {
    runtime.triggerFlag = false;
    runtime.currentValue = 0;
    return;
  }

  if (cfg.triggerDurationMs > 0 &&
      (in.nowMs - runtime.triggerStartMs) >= cfg.triggerDurationMs) {
    runtime.logicalState = false;
    runtime.physicalState = false;
    runtime.triggerFlag = false;
    runtime.currentValue = 0;
    return;
  }
}

bool v3RtcFieldMatches(int fieldValue, int scheduleValue) {
  return scheduleValue < 0 || fieldValue == scheduleValue;
}

bool v3RtcChannelMatchesMinute(const V3RtcScheduleView& channel,
                               const V3RtcMinuteStamp& stamp) {
  if (!channel.enabled) return false;
  return v3RtcFieldMatches(stamp.year, channel.year) &&
         v3RtcFieldMatches(stamp.month, channel.month) &&
         v3RtcFieldMatches(stamp.day, channel.day) &&
         v3RtcFieldMatches(stamp.weekday, channel.weekday) &&
         v3RtcFieldMatches(stamp.hour, channel.hour) &&
         v3RtcFieldMatches(stamp.minute, channel.minute);
}

int32_t v3RtcMinuteKey(const V3RtcMinuteStamp& stamp) {
  return (((stamp.year * 100 + stamp.month) * 100 + stamp.day) * 100 +
          stamp.hour) *
             100 +
         stamp.minute;
}
