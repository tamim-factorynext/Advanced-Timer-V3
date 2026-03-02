#include "kernel/v3_card_bridge.h"
#include "kernel/legacy_card_fields.h"

namespace {
V3CardFamily familyFromLegacy(logicCardType type) {
  switch (type) {
    case DigitalInput:
      return V3CardFamily::DI;
    case DigitalOutput:
      return V3CardFamily::DO;
    case AnalogInput:
      return V3CardFamily::AI;
    case SoftIO:
      return V3CardFamily::SIO;
    case MathCard:
      return V3CardFamily::MATH;
    case RtcCard:
      return V3CardFamily::RTC;
    default:
      return V3CardFamily::DI;
  }
}

void copyCondition(const LogicCard& in, V3ConditionBlock& setOut,
                   V3ConditionBlock& resetOut) {
  setOut.clauseAId = in.setA_ID;
  setOut.clauseAOperator = in.setA_Operator;
  setOut.clauseAThreshold = in.setA_Threshold;
  setOut.clauseBId = in.setB_ID;
  setOut.clauseBOperator = in.setB_Operator;
  setOut.clauseBThreshold = in.setB_Threshold;
  setOut.combiner = in.setCombine;

  resetOut.clauseAId = in.resetA_ID;
  resetOut.clauseAOperator = in.resetA_Operator;
  resetOut.clauseAThreshold = in.resetA_Threshold;
  resetOut.clauseBId = in.resetB_ID;
  resetOut.clauseBOperator = in.resetB_Operator;
  resetOut.clauseBThreshold = in.resetB_Threshold;
  resetOut.combiner = in.resetCombine;
}
}  // namespace

bool legacyToV3CardConfig(const LogicCard& legacy, const int16_t rtcYear,
                          const int8_t rtcMonth, const int8_t rtcDay,
                          const int8_t rtcWeekday, const int8_t rtcHour,
                          const int8_t rtcMinute, V3CardConfig& out) {
  out.cardId = legacy.id;
  out.enabled = true;
  out.faultPolicy = V3FaultPolicy::WARN;
  out.family = familyFromLegacy(legacy.type);

  switch (legacy.type) {
    case DigitalInput: {
      out.di.channel = legacy.index;
      out.di.invert = legacy.invert;
      out.di.debounceTimeMs = legacyDiDebounceMs(legacy);
      out.di.edgeMode = legacy.mode;
      copyCondition(legacy, out.di.set, out.di.reset);
      return true;
    }
    case DigitalOutput: {
      out.dout.channel = legacy.index;
      out.dout.invert = legacy.invert;
      out.dout.mode = legacy.mode;
      out.dout.delayBeforeOnMs = legacyDoDelayBeforeOnMs(legacy);
      out.dout.onDurationMs = legacyDoOnDurationMs(legacy);
      out.dout.repeatCount = legacyDoRepeatCount(legacy);
      copyCondition(legacy, out.dout.set, out.dout.reset);
      return true;
    }
    case AnalogInput: {
      out.ai.channel = legacy.index;
      out.ai.inputMin = legacyAiInputMin(legacy);
      out.ai.inputMax = legacyAiInputMax(legacy);
      out.ai.outputMin = legacyAiOutputMin(legacy);
      out.ai.outputMax = legacyAiOutputMax(legacy);
      uint32_t alphaX100 = (legacyAiAlphaX1000(legacy) + 5U) / 10U;
      if (alphaX100 > 100U) alphaX100 = 100U;
      out.ai.emaAlphaX100 = alphaX100;
      return true;
    }
    case SoftIO: {
      out.sio.invert = legacy.invert;
      out.sio.mode = legacy.mode;
      out.sio.delayBeforeOnMs = legacyDoDelayBeforeOnMs(legacy);
      out.sio.onDurationMs = legacyDoOnDurationMs(legacy);
      out.sio.repeatCount = legacyDoRepeatCount(legacy);
      copyCondition(legacy, out.sio.set, out.sio.reset);
      return true;
    }
    case MathCard: {
      out.math.fallbackValue = legacyMathFallbackValue(legacy);
      out.math.inputA = legacyMathInputA(legacy);
      out.math.inputB = legacyMathInputB(legacy);
      out.math.clampMin = legacyMathClampMin(legacy);
      out.math.clampMax = legacyMathClampMax(legacy);
      copyCondition(legacy, out.math.set, out.math.reset);
      return true;
    }
    case RtcCard: {
      out.rtc.hasYear = rtcYear >= 0;
      out.rtc.hasMonth = rtcMonth >= 0;
      out.rtc.hasDay = rtcDay >= 0;
      out.rtc.hasWeekday = rtcWeekday >= 0;
      out.rtc.year = (rtcYear >= 0) ? static_cast<uint16_t>(rtcYear) : 0U;
      out.rtc.month = (rtcMonth >= 0) ? static_cast<uint8_t>(rtcMonth) : 0U;
      out.rtc.day = (rtcDay >= 0) ? static_cast<uint8_t>(rtcDay) : 0U;
      out.rtc.weekday =
          (rtcWeekday >= 0) ? static_cast<uint8_t>(rtcWeekday) : 0U;
      out.rtc.hour = (rtcHour >= 0) ? static_cast<uint8_t>(rtcHour) : 0U;
      out.rtc.minute =
          (rtcMinute >= 0) ? static_cast<uint8_t>(rtcMinute) : 0U;
      out.rtc.triggerDurationMs = legacyRtcTriggerDurationMs(legacy);
      return true;
    }
    default:
      return false;
  }
}

bool v3CardConfigToLegacy(const V3CardConfig& v3, LogicCard& out) {
  switch (v3.family) {
    case V3CardFamily::DI:
      out.type = DigitalInput;
      out.index = v3.di.channel;
      out.invert = v3.di.invert;
      setLegacyDiDebounceMs(out, v3.di.debounceTimeMs);
      out.mode = v3.di.edgeMode;
      out.setA_ID = v3.di.set.clauseAId;
      out.setA_Operator = v3.di.set.clauseAOperator;
      out.setA_Threshold = v3.di.set.clauseAThreshold;
      out.setB_ID = v3.di.set.clauseBId;
      out.setB_Operator = v3.di.set.clauseBOperator;
      out.setB_Threshold = v3.di.set.clauseBThreshold;
      out.setCombine = v3.di.set.combiner;
      out.resetA_ID = v3.di.reset.clauseAId;
      out.resetA_Operator = v3.di.reset.clauseAOperator;
      out.resetA_Threshold = v3.di.reset.clauseAThreshold;
      out.resetB_ID = v3.di.reset.clauseBId;
      out.resetB_Operator = v3.di.reset.clauseBOperator;
      out.resetB_Threshold = v3.di.reset.clauseBThreshold;
      out.resetCombine = v3.di.reset.combiner;
      return true;
    case V3CardFamily::DO:
      out.type = DigitalOutput;
      out.index = v3.dout.channel;
      out.invert = v3.dout.invert;
      out.mode = v3.dout.mode;
      setLegacyDoDelayBeforeOnMs(out, v3.dout.delayBeforeOnMs);
      setLegacyDoOnDurationMs(out, v3.dout.onDurationMs);
      setLegacyDoRepeatCount(out, v3.dout.repeatCount);
      out.setA_ID = v3.dout.set.clauseAId;
      out.setA_Operator = v3.dout.set.clauseAOperator;
      out.setA_Threshold = v3.dout.set.clauseAThreshold;
      out.setB_ID = v3.dout.set.clauseBId;
      out.setB_Operator = v3.dout.set.clauseBOperator;
      out.setB_Threshold = v3.dout.set.clauseBThreshold;
      out.setCombine = v3.dout.set.combiner;
      out.resetA_ID = v3.dout.reset.clauseAId;
      out.resetA_Operator = v3.dout.reset.clauseAOperator;
      out.resetA_Threshold = v3.dout.reset.clauseAThreshold;
      out.resetB_ID = v3.dout.reset.clauseBId;
      out.resetB_Operator = v3.dout.reset.clauseBOperator;
      out.resetB_Threshold = v3.dout.reset.clauseBThreshold;
      out.resetCombine = v3.dout.reset.combiner;
      return true;
    case V3CardFamily::AI:
      out.type = AnalogInput;
      out.index = v3.ai.channel;
      setLegacyAiInputMin(out, v3.ai.inputMin);
      setLegacyAiInputMax(out, v3.ai.inputMax);
      setLegacyAiOutputMin(out, v3.ai.outputMin);
      setLegacyAiOutputMax(out, v3.ai.outputMax);
      setLegacyAiAlphaX1000(out, v3.ai.emaAlphaX100 * 10U);
      out.mode = Mode_AI_Continuous;
      return true;
    case V3CardFamily::SIO:
      out.type = SoftIO;
      out.invert = v3.sio.invert;
      out.mode = v3.sio.mode;
      setLegacyDoDelayBeforeOnMs(out, v3.sio.delayBeforeOnMs);
      setLegacyDoOnDurationMs(out, v3.sio.onDurationMs);
      setLegacyDoRepeatCount(out, v3.sio.repeatCount);
      out.setA_ID = v3.sio.set.clauseAId;
      out.setA_Operator = v3.sio.set.clauseAOperator;
      out.setA_Threshold = v3.sio.set.clauseAThreshold;
      out.setB_ID = v3.sio.set.clauseBId;
      out.setB_Operator = v3.sio.set.clauseBOperator;
      out.setB_Threshold = v3.sio.set.clauseBThreshold;
      out.setCombine = v3.sio.set.combiner;
      out.resetA_ID = v3.sio.reset.clauseAId;
      out.resetA_Operator = v3.sio.reset.clauseAOperator;
      out.resetA_Threshold = v3.sio.reset.clauseAThreshold;
      out.resetB_ID = v3.sio.reset.clauseBId;
      out.resetB_Operator = v3.sio.reset.clauseBOperator;
      out.resetB_Threshold = v3.sio.reset.clauseBThreshold;
      out.resetCombine = v3.sio.reset.combiner;
      return true;
    case V3CardFamily::MATH:
      out.type = MathCard;
      setLegacyMathFallbackValue(out, v3.math.fallbackValue);
      setLegacyMathInputA(out, v3.math.inputA);
      setLegacyMathInputB(out, v3.math.inputB);
      setLegacyMathClampMin(out, v3.math.clampMin);
      setLegacyMathClampMax(out, v3.math.clampMax);
      out.mode = Mode_None;
      out.setA_ID = v3.math.set.clauseAId;
      out.setA_Operator = v3.math.set.clauseAOperator;
      out.setA_Threshold = v3.math.set.clauseAThreshold;
      out.setB_ID = v3.math.set.clauseBId;
      out.setB_Operator = v3.math.set.clauseBOperator;
      out.setB_Threshold = v3.math.set.clauseBThreshold;
      out.setCombine = v3.math.set.combiner;
      out.resetA_ID = v3.math.reset.clauseAId;
      out.resetA_Operator = v3.math.reset.clauseAOperator;
      out.resetA_Threshold = v3.math.reset.clauseAThreshold;
      out.resetB_ID = v3.math.reset.clauseBId;
      out.resetB_Operator = v3.math.reset.clauseBOperator;
      out.resetB_Threshold = v3.math.reset.clauseBThreshold;
      out.resetCombine = v3.math.reset.combiner;
      return true;
    case V3CardFamily::RTC:
      out.type = RtcCard;
      out.mode = Mode_None;
      setLegacyRtcTriggerDurationMs(out, v3.rtc.triggerDurationMs);
      out.setA_Operator = Op_AlwaysFalse;
      out.setB_Operator = Op_AlwaysFalse;
      out.resetA_Operator = Op_AlwaysFalse;
      out.resetB_Operator = Op_AlwaysFalse;
      out.setCombine = Combine_None;
      out.resetCombine = Combine_None;
      return true;
    default:
      return false;
  }
}
