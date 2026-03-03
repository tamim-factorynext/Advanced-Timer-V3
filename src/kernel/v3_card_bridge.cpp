/*
File: src/kernel/v3_card_bridge.cpp
Purpose: Implements the v3 card bridge module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/v3_card_bridge.cpp
- src/kernel/v3_card_bridge.h

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
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
      copyCondition(legacy, out.di.turnOnCondition, out.di.turnOffCondition);
      return true;
    }
    case DigitalOutput: {
      out.dout.channel = legacy.index;
      out.dout.invert = legacy.invert;
      out.dout.mode = legacy.mode;
      out.dout.delayBeforeOnMs = legacyDoDelayBeforeOnMs(legacy);
      out.dout.onDurationMs = legacyDoOnDurationMs(legacy);
      out.dout.repeatCount = legacyDoRepeatCount(legacy);
      copyCondition(legacy, out.dout.turnOnCondition, out.dout.turnOffCondition);
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
      out.ai.smoothingFactorPct = alphaX100;
      return true;
    }
    case SoftIO: {
      out.sio.invert = legacy.invert;
      out.sio.mode = legacy.mode;
      out.sio.delayBeforeOnMs = legacyDoDelayBeforeOnMs(legacy);
      out.sio.onDurationMs = legacyDoOnDurationMs(legacy);
      out.sio.repeatCount = legacyDoRepeatCount(legacy);
      copyCondition(legacy, out.sio.turnOnCondition, out.sio.turnOffCondition);
      return true;
    }
    case MathCard: {
      out.math.operation = 0U;
      out.math.fallbackValue = legacyMathFallbackValue(legacy);
      out.math.inputA = legacyMathInputA(legacy);
      out.math.inputB = legacyMathInputB(legacy);
      out.math.inputMin = legacyMathClampMin(legacy);
      out.math.inputMax = legacyMathClampMax(legacy);
      if (out.math.inputMin >= out.math.inputMax) {
        out.math.inputMin = 0U;
        out.math.inputMax = 10000U;
      }
      out.math.outputMin = out.math.inputMin;
      out.math.outputMax = out.math.inputMax;
      out.math.smoothingFactorPct = 100U;
      copyCondition(legacy, out.math.turnOnCondition,
                    out.math.turnOffCondition);
      return true;
    }
    case RtcCard: {
      out.rtc.hasYear = rtcYear >= 0;
      out.rtc.hasMonth = rtcMonth >= 0;
      out.rtc.hasDay = rtcDay >= 0;
      out.rtc.hasWeekday = rtcWeekday >= 0;
      out.rtc.hasHour = rtcHour >= 0;
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
      out.setA_ID = v3.di.turnOnCondition.clauseAId;
      out.setA_Operator = v3.di.turnOnCondition.clauseAOperator;
      out.setA_Threshold = v3.di.turnOnCondition.clauseAThreshold;
      out.setB_ID = v3.di.turnOnCondition.clauseBId;
      out.setB_Operator = v3.di.turnOnCondition.clauseBOperator;
      out.setB_Threshold = v3.di.turnOnCondition.clauseBThreshold;
      out.setCombine = v3.di.turnOnCondition.combiner;
      out.resetA_ID = v3.di.turnOffCondition.clauseAId;
      out.resetA_Operator = v3.di.turnOffCondition.clauseAOperator;
      out.resetA_Threshold = v3.di.turnOffCondition.clauseAThreshold;
      out.resetB_ID = v3.di.turnOffCondition.clauseBId;
      out.resetB_Operator = v3.di.turnOffCondition.clauseBOperator;
      out.resetB_Threshold = v3.di.turnOffCondition.clauseBThreshold;
      out.resetCombine = v3.di.turnOffCondition.combiner;
      return true;
    case V3CardFamily::DO:
      out.type = DigitalOutput;
      out.index = v3.dout.channel;
      out.invert = v3.dout.invert;
      out.mode = v3.dout.mode;
      setLegacyDoDelayBeforeOnMs(out, v3.dout.delayBeforeOnMs);
      setLegacyDoOnDurationMs(out, v3.dout.onDurationMs);
      setLegacyDoRepeatCount(out, v3.dout.repeatCount);
      out.setA_ID = v3.dout.turnOnCondition.clauseAId;
      out.setA_Operator = v3.dout.turnOnCondition.clauseAOperator;
      out.setA_Threshold = v3.dout.turnOnCondition.clauseAThreshold;
      out.setB_ID = v3.dout.turnOnCondition.clauseBId;
      out.setB_Operator = v3.dout.turnOnCondition.clauseBOperator;
      out.setB_Threshold = v3.dout.turnOnCondition.clauseBThreshold;
      out.setCombine = v3.dout.turnOnCondition.combiner;
      out.resetA_ID = v3.dout.turnOffCondition.clauseAId;
      out.resetA_Operator = v3.dout.turnOffCondition.clauseAOperator;
      out.resetA_Threshold = v3.dout.turnOffCondition.clauseAThreshold;
      out.resetB_ID = v3.dout.turnOffCondition.clauseBId;
      out.resetB_Operator = v3.dout.turnOffCondition.clauseBOperator;
      out.resetB_Threshold = v3.dout.turnOffCondition.clauseBThreshold;
      out.resetCombine = v3.dout.turnOffCondition.combiner;
      return true;
    case V3CardFamily::AI:
      out.type = AnalogInput;
      out.index = v3.ai.channel;
      setLegacyAiInputMin(out, v3.ai.inputMin);
      setLegacyAiInputMax(out, v3.ai.inputMax);
      setLegacyAiOutputMin(out, v3.ai.outputMin);
      setLegacyAiOutputMax(out, v3.ai.outputMax);
      setLegacyAiAlphaX1000(out, v3.ai.smoothingFactorPct * 10U);
      out.mode = Mode_AI_Continuous;
      return true;
    case V3CardFamily::SIO:
      out.type = SoftIO;
      out.invert = v3.sio.invert;
      out.mode = v3.sio.mode;
      setLegacyDoDelayBeforeOnMs(out, v3.sio.delayBeforeOnMs);
      setLegacyDoOnDurationMs(out, v3.sio.onDurationMs);
      setLegacyDoRepeatCount(out, v3.sio.repeatCount);
      out.setA_ID = v3.sio.turnOnCondition.clauseAId;
      out.setA_Operator = v3.sio.turnOnCondition.clauseAOperator;
      out.setA_Threshold = v3.sio.turnOnCondition.clauseAThreshold;
      out.setB_ID = v3.sio.turnOnCondition.clauseBId;
      out.setB_Operator = v3.sio.turnOnCondition.clauseBOperator;
      out.setB_Threshold = v3.sio.turnOnCondition.clauseBThreshold;
      out.setCombine = v3.sio.turnOnCondition.combiner;
      out.resetA_ID = v3.sio.turnOffCondition.clauseAId;
      out.resetA_Operator = v3.sio.turnOffCondition.clauseAOperator;
      out.resetA_Threshold = v3.sio.turnOffCondition.clauseAThreshold;
      out.resetB_ID = v3.sio.turnOffCondition.clauseBId;
      out.resetB_Operator = v3.sio.turnOffCondition.clauseBOperator;
      out.resetB_Threshold = v3.sio.turnOffCondition.clauseBThreshold;
      out.resetCombine = v3.sio.turnOffCondition.combiner;
      return true;
    case V3CardFamily::MATH:
      out.type = MathCard;
      setLegacyMathFallbackValue(out, v3.math.fallbackValue);
      setLegacyMathInputA(out, v3.math.inputA);
      setLegacyMathInputB(out, v3.math.inputB);
      setLegacyMathClampMin(out, v3.math.inputMin);
      setLegacyMathClampMax(out, v3.math.inputMax);
      out.mode = Mode_None;
      out.setA_ID = v3.math.turnOnCondition.clauseAId;
      out.setA_Operator = v3.math.turnOnCondition.clauseAOperator;
      out.setA_Threshold = v3.math.turnOnCondition.clauseAThreshold;
      out.setB_ID = v3.math.turnOnCondition.clauseBId;
      out.setB_Operator = v3.math.turnOnCondition.clauseBOperator;
      out.setB_Threshold = v3.math.turnOnCondition.clauseBThreshold;
      out.setCombine = v3.math.turnOnCondition.combiner;
      out.resetA_ID = v3.math.turnOffCondition.clauseAId;
      out.resetA_Operator = v3.math.turnOffCondition.clauseAOperator;
      out.resetA_Threshold = v3.math.turnOffCondition.clauseAThreshold;
      out.resetB_ID = v3.math.turnOffCondition.clauseBId;
      out.resetB_Operator = v3.math.turnOffCondition.clauseBOperator;
      out.resetB_Threshold = v3.math.turnOffCondition.clauseBThreshold;
      out.resetCombine = v3.math.turnOffCondition.combiner;
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


