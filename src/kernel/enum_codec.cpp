#include "kernel/enum_codec.h"

#include <cctype>
#include <cstring>

namespace {
bool enumTokenEquals(const char* s, const char* token) {
  if (s == nullptr || token == nullptr) return false;
  char cleaned[64];
  size_t j = 0;
  for (size_t i = 0; s[i] != '\0' && j < (sizeof(cleaned) - 1); ++i) {
    const unsigned char ch = static_cast<unsigned char>(s[i]);
    if (std::isalnum(ch) || ch == '_') cleaned[j++] = static_cast<char>(ch);
  }
  cleaned[j] = '\0';
  return std::strcmp(cleaned, token) == 0;
}
}  // namespace

const char* toString(logicCardType value) {
  switch (value) {
    case DigitalInput:
      return "DigitalInput";
    case DigitalOutput:
      return "DigitalOutput";
    case AnalogInput:
      return "AnalogInput";
    case SoftIO:
      return "SoftIO";
    default:
      return "DigitalInput";
  }
}

const char* toString(logicOperator value) {
  switch (value) {
    case Op_AlwaysTrue:
      return "Op_AlwaysTrue";
    case Op_AlwaysFalse:
      return "Op_AlwaysFalse";
    case Op_LogicalTrue:
      return "Op_LogicalTrue";
    case Op_LogicalFalse:
      return "Op_LogicalFalse";
    case Op_PhysicalOn:
      return "Op_PhysicalOn";
    case Op_PhysicalOff:
      return "Op_PhysicalOff";
    case Op_Triggered:
      return "Op_Triggered";
    case Op_TriggerCleared:
      return "Op_TriggerCleared";
    case Op_GT:
      return "Op_GT";
    case Op_LT:
      return "Op_LT";
    case Op_EQ:
      return "Op_EQ";
    case Op_NEQ:
      return "Op_NEQ";
    case Op_GTE:
      return "Op_GTE";
    case Op_LTE:
      return "Op_LTE";
    case Op_Running:
      return "Op_Running";
    case Op_Finished:
      return "Op_Finished";
    case Op_Stopped:
      return "Op_Stopped";
    default:
      return "Op_AlwaysTrue";
  }
}

const char* toString(cardMode value) {
  switch (value) {
    case Mode_None:
      return "Mode_None";
    case Mode_DI_Rising:
      return "Mode_DI_Rising";
    case Mode_DI_Falling:
      return "Mode_DI_Falling";
    case Mode_DI_Change:
      return "Mode_DI_Change";
    case Mode_AI_Continuous:
      return "Mode_AI_Continuous";
    case Mode_DO_Normal:
      return "Mode_DO_Normal";
    case Mode_DO_Immediate:
      return "Mode_DO_Immediate";
    case Mode_DO_Gated:
      return "Mode_DO_Gated";
    default:
      return "Mode_None";
  }
}

const char* toString(cardState value) {
  switch (value) {
    case State_None:
      return "State_None";
    case State_DI_Idle:
      return "State_DI_Idle";
    case State_DI_Filtering:
      return "State_DI_Filtering";
    case State_DI_Qualified:
      return "State_DI_Qualified";
    case State_DI_Inhibited:
      return "State_DI_Inhibited";
    case State_AI_Streaming:
      return "State_AI_Streaming";
    case State_DO_Idle:
      return "State_DO_Idle";
    case State_DO_OnDelay:
      return "State_DO_OnDelay";
    case State_DO_Active:
      return "State_DO_Active";
    case State_DO_Finished:
      return "State_DO_Finished";
    default:
      return "State_None";
  }
}

const char* toString(combineMode value) {
  switch (value) {
    case Combine_None:
      return "Combine_None";
    case Combine_AND:
      return "Combine_AND";
    case Combine_OR:
      return "Combine_OR";
    default:
      return "Combine_None";
  }
}

const char* toString(runMode value) {
  switch (value) {
    case RUN_NORMAL:
      return "RUN_NORMAL";
    case RUN_STEP:
      return "RUN_STEP";
    case RUN_BREAKPOINT:
      return "RUN_BREAKPOINT";
    case RUN_SLOW:
      return "RUN_SLOW";
    default:
      return "RUN_NORMAL";
  }
}

const char* toString(inputSourceMode value) {
  switch (value) {
    case InputSource_Real:
      return "REAL";
    case InputSource_ForcedHigh:
      return "FORCED_HIGH";
    case InputSource_ForcedLow:
      return "FORCED_LOW";
    case InputSource_ForcedValue:
      return "FORCED_VALUE";
    default:
      return "REAL";
  }
}

bool tryParseLogicCardType(const char* s, logicCardType& out) {
  if (s == nullptr) return false;
  if (enumTokenEquals(s, "DigitalInput")) return (out = DigitalInput), true;
  if (enumTokenEquals(s, "DigitalOutput")) return (out = DigitalOutput), true;
  if (enumTokenEquals(s, "AnalogInput")) return (out = AnalogInput), true;
  if (enumTokenEquals(s, "SoftIO")) return (out = SoftIO), true;
  return false;
}

bool tryParseLogicOperator(const char* s, logicOperator& out) {
  if (s == nullptr) return false;
  if (enumTokenEquals(s, "Op_AlwaysTrue")) return (out = Op_AlwaysTrue), true;
  if (enumTokenEquals(s, "Op_AlwaysFalse")) return (out = Op_AlwaysFalse), true;
  if (enumTokenEquals(s, "Op_LogicalTrue")) return (out = Op_LogicalTrue), true;
  if (enumTokenEquals(s, "Op_LogicalFalse")) return (out = Op_LogicalFalse), true;
  if (enumTokenEquals(s, "Op_PhysicalOn")) return (out = Op_PhysicalOn), true;
  if (enumTokenEquals(s, "Op_PhysicalOff")) return (out = Op_PhysicalOff), true;
  if (enumTokenEquals(s, "Op_Triggered")) return (out = Op_Triggered), true;
  if (enumTokenEquals(s, "Op_TriggerCleared"))
    return (out = Op_TriggerCleared), true;
  if (enumTokenEquals(s, "Op_GT")) return (out = Op_GT), true;
  if (enumTokenEquals(s, "Op_LT")) return (out = Op_LT), true;
  if (enumTokenEquals(s, "Op_EQ")) return (out = Op_EQ), true;
  if (enumTokenEquals(s, "Op_NEQ")) return (out = Op_NEQ), true;
  if (enumTokenEquals(s, "Op_GTE")) return (out = Op_GTE), true;
  if (enumTokenEquals(s, "Op_LTE")) return (out = Op_LTE), true;
  if (enumTokenEquals(s, "Op_Running")) return (out = Op_Running), true;
  if (enumTokenEquals(s, "Op_Finished")) return (out = Op_Finished), true;
  if (enumTokenEquals(s, "Op_Stopped")) return (out = Op_Stopped), true;
  return false;
}

bool tryParseCardMode(const char* s, cardMode& out) {
  if (s == nullptr) return false;
  if (enumTokenEquals(s, "Mode_None")) return (out = Mode_None), true;
  if (enumTokenEquals(s, "Mode_DI_Rising")) return (out = Mode_DI_Rising), true;
  if (enumTokenEquals(s, "Mode_DI_Falling"))
    return (out = Mode_DI_Falling), true;
  if (enumTokenEquals(s, "Mode_DI_Change")) return (out = Mode_DI_Change), true;
  if (enumTokenEquals(s, "Mode_AI_Continuous"))
    return (out = Mode_AI_Continuous), true;
  if (enumTokenEquals(s, "Mode_DO_Normal")) return (out = Mode_DO_Normal), true;
  if (enumTokenEquals(s, "Mode_DO_Immediate"))
    return (out = Mode_DO_Immediate), true;
  if (enumTokenEquals(s, "Mode_DO_Gated")) return (out = Mode_DO_Gated), true;
  return false;
}

bool tryParseCardState(const char* s, cardState& out) {
  if (s == nullptr) return false;
  if (enumTokenEquals(s, "State_None")) return (out = State_None), true;
  if (enumTokenEquals(s, "State_DI_Idle")) return (out = State_DI_Idle), true;
  if (enumTokenEquals(s, "State_DI_Filtering"))
    return (out = State_DI_Filtering), true;
  if (enumTokenEquals(s, "State_DI_Qualified"))
    return (out = State_DI_Qualified), true;
  if (enumTokenEquals(s, "State_DI_Inhibited"))
    return (out = State_DI_Inhibited), true;
  if (enumTokenEquals(s, "State_AI_Streaming"))
    return (out = State_AI_Streaming), true;
  if (enumTokenEquals(s, "State_DO_Idle")) return (out = State_DO_Idle), true;
  if (enumTokenEquals(s, "State_DO_OnDelay"))
    return (out = State_DO_OnDelay), true;
  if (enumTokenEquals(s, "State_DO_Active")) return (out = State_DO_Active), true;
  if (enumTokenEquals(s, "State_DO_Finished"))
    return (out = State_DO_Finished), true;
  return false;
}

bool tryParseCombineMode(const char* s, combineMode& out) {
  if (s == nullptr) return false;
  if (enumTokenEquals(s, "Combine_None")) return (out = Combine_None), true;
  if (enumTokenEquals(s, "Combine_AND")) return (out = Combine_AND), true;
  if (enumTokenEquals(s, "Combine_OR")) return (out = Combine_OR), true;
  return false;
}

logicCardType parseOrDefault(const char* s, logicCardType fallback) {
  logicCardType value = fallback;
  if (tryParseLogicCardType(s, value)) return value;
  return fallback;
}

logicOperator parseOrDefault(const char* s, logicOperator fallback) {
  logicOperator value = fallback;
  if (tryParseLogicOperator(s, value)) return value;
  return fallback;
}

cardMode parseOrDefault(const char* s, cardMode fallback) {
  cardMode value = fallback;
  if (tryParseCardMode(s, value)) return value;
  return fallback;
}

cardState parseOrDefault(const char* s, cardState fallback) {
  cardState value = fallback;
  if (tryParseCardState(s, value)) return value;
  return fallback;
}

combineMode parseOrDefault(const char* s, combineMode fallback) {
  combineMode value = fallback;
  if (tryParseCombineMode(s, value)) return value;
  return fallback;
}
