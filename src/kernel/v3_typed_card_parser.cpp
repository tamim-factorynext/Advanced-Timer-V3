/*
File: src/kernel/v3_typed_card_parser.cpp
Purpose: Implements the v3 typed card parser module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/v3_typed_card_parser.cpp
- src/kernel/v3_typed_card_parser.h
- src/storage/v3_normalizer.cpp

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_typed_card_parser.h"

#include <cstring>

#include "kernel/v3_condition_rules.h"

namespace {
logicCardType expectedTypeForCardId(uint8_t id, uint8_t doStart, uint8_t aiStart,
                                    uint8_t sioStart, uint8_t mathStart,
                                    uint8_t rtcStart) {
  if (id < doStart) return DigitalInput;
  if (id < aiStart) return DigitalOutput;
  if (id < sioStart) return AnalogInput;
  if (id < mathStart) return SoftIO;
  if (id < rtcStart) return MathCard;
  return RtcCard;
}

bool mapV3ModeToLegacy(logicCardType type, const char* mode, cardMode& outMode) {
  if (type == DigitalInput) {
    if (std::strcmp(mode, "RISING") == 0) return (outMode = Mode_DI_Rising), true;
    if (std::strcmp(mode, "FALLING") == 0)
      return (outMode = Mode_DI_Falling), true;
    if (std::strcmp(mode, "CHANGE") == 0) return (outMode = Mode_DI_Change), true;
    return false;
  }
  if (type == DigitalOutput || type == SoftIO) {
    if (std::strcmp(mode, "Normal") == 0) return (outMode = Mode_DO_Normal), true;
    if (std::strcmp(mode, "Immediate") == 0)
      return (outMode = Mode_DO_Immediate), true;
    if (std::strcmp(mode, "Gated") == 0) return (outMode = Mode_DO_Gated), true;
    return false;
  }
  if (type == AnalogInput) return (outMode = Mode_AI_Continuous), true;
  return (outMode = Mode_None), true;
}

bool parseMathOperator(const char* op, uint8_t& outOp) {
  if (op == nullptr) return false;
  if (std::strcmp(op, "ADD") == 0) return (outOp = 0U), true;
  if (std::strcmp(op, "SUB_SAT") == 0) return (outOp = 1U), true;
  if (std::strcmp(op, "MUL") == 0) return (outOp = 2U), true;
  if (std::strcmp(op, "DIV_SAFE") == 0) return (outOp = 3U), true;
  return false;
}

bool parseThresholdAsUInt(JsonVariantConst threshold, uint32_t& out) {
  if (threshold.is<uint64_t>()) {
    uint64_t v = threshold.as<uint64_t>();
    if (v > UINT32_MAX) return false;
    out = static_cast<uint32_t>(v);
    return true;
  }
  if (threshold.is<int64_t>()) {
    int64_t v = threshold.as<int64_t>();
    if (v < 0 || v > static_cast<int64_t>(UINT32_MAX)) return false;
    out = static_cast<uint32_t>(v);
    return true;
  }
  if (threshold.is<bool>()) {
    out = threshold.as<bool>() ? 1U : 0U;
    return true;
  }
  return false;
}

logicOperator mapV3ClauseToLegacyOperator(logicCardType sourceType,
                                          const char* field, const char* op,
                                          JsonVariantConst threshold,
                                          uint32_t& ioThreshold, bool& ok) {
  ok = true;
  if (!isV3FieldAllowedForSourceType(sourceType, field) ||
      !isV3OperatorAllowedForField(field, op)) {
    ok = false;
    return Op_AlwaysFalse;
  }

  if (std::strcmp(field, "missionState") == 0) {
    const char* stateName = threshold.as<const char*>();
    if (std::strcmp(op, "EQ") != 0 || stateName == nullptr) {
      ok = false;
      return Op_AlwaysFalse;
    }
    ioThreshold = 0;
    if (std::strcmp(stateName, "IDLE") == 0) return Op_Stopped;
    if (std::strcmp(stateName, "ACTIVE") == 0) return Op_Running;
    if (std::strcmp(stateName, "FINISHED") == 0) return Op_Finished;
    ok = false;
    return Op_AlwaysFalse;
  }

  uint32_t numeric = 0;
  if (!parseThresholdAsUInt(threshold, numeric)) {
    ok = false;
    return Op_AlwaysFalse;
  }
  ioThreshold = numeric;

  if (std::strcmp(field, "commandState") == 0 ||
      std::strcmp(field, "actualState") == 0) {
    if (std::strcmp(op, "EQ") == 0) {
      if (std::strcmp(field, "commandState") == 0) {
        return (numeric != 0) ? Op_LogicalTrue : Op_LogicalFalse;
      }
      return (numeric != 0) ? Op_PhysicalOn : Op_PhysicalOff;
    }
    if (std::strcmp(op, "NEQ") == 0) {
      if (std::strcmp(field, "commandState") == 0) {
        return (numeric != 0) ? Op_LogicalFalse : Op_LogicalTrue;
      }
      return (numeric != 0) ? Op_PhysicalOff : Op_PhysicalOn;
    }
  }

  if (std::strcmp(field, "edgePulse") == 0) {
    if (std::strcmp(op, "EQ") == 0) {
      return (numeric != 0) ? Op_Triggered : Op_TriggerCleared;
    }
    if (std::strcmp(op, "NEQ") == 0) {
      return (numeric != 0) ? Op_TriggerCleared : Op_Triggered;
    }
  }

  if (std::strcmp(op, "GT") == 0) return Op_GT;
  if (std::strcmp(op, "GTE") == 0) return Op_GTE;
  if (std::strcmp(op, "LT") == 0) return Op_LT;
  if (std::strcmp(op, "LTE") == 0) return Op_LTE;
  if (std::strcmp(op, "EQ") == 0) return Op_EQ;
  if (std::strcmp(op, "NEQ") == 0) return Op_NEQ;
  ok = false;
  return Op_AlwaysFalse;
}

bool mapV3ConditionBlock(JsonObjectConst block, uint8_t totalCards,
                         uint8_t& outAId, logicOperator& outAOp,
                         uint32_t& outAThreshold, uint8_t& outBId,
                         logicOperator& outBOp, uint32_t& outBThreshold,
                         combineMode& outCombine, String& reason,
                         const logicCardType* sourceTypeById) {
  const char* combiner = block["combiner"] | "NONE";
  if (std::strcmp(combiner, "NONE") == 0) {
    outCombine = Combine_None;
  } else if (std::strcmp(combiner, "AND") == 0) {
    outCombine = Combine_AND;
  } else if (std::strcmp(combiner, "OR") == 0) {
    outCombine = Combine_OR;
  } else {
    reason = "invalid condition combiner";
    return false;
  }

  if (!block["clauseA"].is<JsonObjectConst>()) {
    reason = "missing clauseA";
    return false;
  }
  JsonObjectConst a = block["clauseA"].as<JsonObjectConst>();
  if (!a["source"].is<JsonObjectConst>()) {
    reason = "clauseA.source missing";
    return false;
  }
  JsonObjectConst aSource = a["source"].as<JsonObjectConst>();
  outAId = aSource["cardId"] | 255;
  if (outAId >= totalCards) {
    reason = "clauseA source cardId out of range";
    return false;
  }
  const char* aField = aSource["field"] | "liveValue";
  const char* aOperator = a["operator"] | "";
  bool aOk = false;
  outAOp = mapV3ClauseToLegacyOperator(sourceTypeById[outAId], aField, aOperator,
                                       a["threshold"], outAThreshold, aOk);
  if (!aOk) {
    reason = "invalid clauseA field/operator/threshold for source card type";
    return false;
  }

  outBId = outAId;
  outBOp = Op_AlwaysFalse;
  outBThreshold = 0;
  if (outCombine == Combine_None) return true;

  if (!block["clauseB"].is<JsonObjectConst>()) {
    reason = "missing clauseB";
    return false;
  }
  JsonObjectConst b = block["clauseB"].as<JsonObjectConst>();
  if (!b["source"].is<JsonObjectConst>()) {
    reason = "clauseB.source missing";
    return false;
  }
  JsonObjectConst bSource = b["source"].as<JsonObjectConst>();
  outBId = bSource["cardId"] | 255;
  if (outBId >= totalCards) {
    reason = "clauseB source cardId out of range";
    return false;
  }
  const char* bField = bSource["field"] | "liveValue";
  const char* bOperator = b["operator"] | "";
  bool bOk = false;
  outBOp = mapV3ClauseToLegacyOperator(sourceTypeById[outBId], bField, bOperator,
                                       b["threshold"], outBThreshold, bOk);
  if (!bOk) {
    reason = "invalid clauseB field/operator/threshold for source card type";
    return false;
  }
  return true;
}

V3CardFamily v3FamilyFromLogicType(logicCardType type) {
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
}  // namespace

bool parseV3CardToTyped(JsonObjectConst v3Card, const logicCardType* sourceTypeById,
                        uint8_t totalCards, uint8_t doStart, uint8_t aiStart,
                        uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart,
                        V3CardConfig& out, String& reason) {
  const uint8_t cardId = v3Card["cardId"] | 255;
  if (cardId >= totalCards) {
    reason = "cardId out of range";
    return false;
  }
  const char* cardType = v3Card["cardType"] | "";
  const logicCardType expectedType =
      expectedTypeForCardId(cardId, doStart, aiStart, sioStart, mathStart,
                            rtcStart);
  logicCardType parsedType = DigitalInput;
  if (!parseV3CardTypeToken(cardType, parsedType)) {
    reason = "invalid cardType";
    return false;
  }
  if (parsedType != expectedType) {
    reason = "cardType does not match cardId family slot";
    return false;
  }

  out.cardId = cardId;
  out.family = v3FamilyFromLogicType(expectedType);
  out.enabled = v3Card["enabled"] | true;
  out.faultPolicy = V3FaultPolicy::WARN;

  const char* faultPolicy = v3Card["faultPolicy"] | "WARN";
  if (std::strcmp(faultPolicy, "INFO") == 0) out.faultPolicy = V3FaultPolicy::INFO;
  if (std::strcmp(faultPolicy, "CRITICAL") == 0)
    out.faultPolicy = V3FaultPolicy::CRITICAL;

  JsonObjectConst cfg = v3Card["config"].as<JsonObjectConst>();
  if (cfg.isNull()) {
    reason = "missing card config";
    return false;
  }

  if (expectedType == DigitalInput) {
    out.di.channel = cfg["channel"] | cardId;
    out.di.invert = cfg["invert"] | false;
    out.di.debounceTimeMs = cfg["debounceTime"] | 0U;
    const char* edgeMode = cfg["edgeMode"] | "RISING";
    cardMode diMode = Mode_DI_Rising;
    if (!mapV3ModeToLegacy(expectedType, edgeMode, diMode)) {
      reason = "invalid DI edgeMode";
      return false;
    }
    out.di.edgeMode = diMode;
    if (!cfg["turnOnCondition"].is<JsonObjectConst>() || !cfg["turnOffCondition"].is<JsonObjectConst>()) {
      reason = "DI require set and reset blocks";
      return false;
    }
    if (!mapV3ConditionBlock(cfg["turnOnCondition"].as<JsonObjectConst>(), totalCards,
                             out.di.turnOnCondition.clauseAId, out.di.turnOnCondition.clauseAOperator,
                             out.di.turnOnCondition.clauseAThreshold, out.di.turnOnCondition.clauseBId,
                             out.di.turnOnCondition.clauseBOperator,
                             out.di.turnOnCondition.clauseBThreshold, out.di.turnOnCondition.combiner,
                             reason, sourceTypeById)) {
      return false;
    }
    if (!mapV3ConditionBlock(cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
                             out.di.turnOffCondition.clauseAId, out.di.turnOffCondition.clauseAOperator,
                             out.di.turnOffCondition.clauseAThreshold, out.di.turnOffCondition.clauseBId,
                             out.di.turnOffCondition.clauseBOperator,
                             out.di.turnOffCondition.clauseBThreshold,
                             out.di.turnOffCondition.combiner, reason, sourceTypeById)) {
      return false;
    }
    return true;
  }

  if (expectedType == AnalogInput) {
    JsonObjectConst inputRange = cfg["inputRange"].as<JsonObjectConst>();
    JsonObjectConst outputRange = cfg["outputRange"].as<JsonObjectConst>();
    out.ai.channel = cfg["channel"] | cardId;
    out.ai.inputMin = inputRange["min"] | 4U;
    out.ai.inputMax = inputRange["max"] | 20U;
    const uint32_t smoothingFactorPct = cfg["emaAlpha"] | 100U;
    if (smoothingFactorPct > 100U) {
      reason = "AI emaAlpha out of range";
      return false;
    }
    out.ai.smoothingFactorPct = smoothingFactorPct;
    out.ai.outputMin = outputRange["min"] | 0U;
    out.ai.outputMax = outputRange["max"] | 100U;
    return true;
  }

  if (expectedType == DigitalOutput || expectedType == SoftIO) {
    const char* mode = cfg["mode"] | "Normal";
    cardMode outMode = Mode_DO_Normal;
    if (!mapV3ModeToLegacy(expectedType, mode, outMode)) {
      reason = "invalid DO/SIO mode";
      return false;
    }
    if (expectedType == DigitalOutput) {
      out.dout.channel = cfg["channel"] | cardId;
      out.dout.invert = cfg["invert"] | false;
      out.dout.mode = outMode;
      out.dout.delayBeforeOnMs = cfg["delayBeforeON"] | 0U;
      out.dout.onDurationMs = cfg["onDuration"] | 0U;
      out.dout.repeatCount = cfg["repeatCount"] | 1U;
    } else {
      out.sio.invert = cfg["invert"] | false;
      out.sio.mode = outMode;
      out.sio.delayBeforeOnMs = cfg["delayBeforeON"] | 0U;
      out.sio.onDurationMs = cfg["onDuration"] | 0U;
      out.sio.repeatCount = cfg["repeatCount"] | 1U;
    }
    if (!cfg["turnOnCondition"].is<JsonObjectConst>() || !cfg["turnOffCondition"].is<JsonObjectConst>()) {
      reason = "DO/SIO require set and reset blocks";
      return false;
    }
    if (expectedType == DigitalOutput) {
      if (!mapV3ConditionBlock(
              cfg["turnOnCondition"].as<JsonObjectConst>(), totalCards, out.dout.turnOnCondition.clauseAId,
              out.dout.turnOnCondition.clauseAOperator, out.dout.turnOnCondition.clauseAThreshold,
              out.dout.turnOnCondition.clauseBId, out.dout.turnOnCondition.clauseBOperator,
              out.dout.turnOnCondition.clauseBThreshold, out.dout.turnOnCondition.combiner, reason,
              sourceTypeById)) {
        return false;
      }
      if (!mapV3ConditionBlock(
              cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
              out.dout.turnOffCondition.clauseAId, out.dout.turnOffCondition.clauseAOperator,
              out.dout.turnOffCondition.clauseAThreshold, out.dout.turnOffCondition.clauseBId,
              out.dout.turnOffCondition.clauseBOperator, out.dout.turnOffCondition.clauseBThreshold,
              out.dout.turnOffCondition.combiner, reason, sourceTypeById)) {
        return false;
      }
    } else {
      if (!mapV3ConditionBlock(
              cfg["turnOnCondition"].as<JsonObjectConst>(), totalCards, out.sio.turnOnCondition.clauseAId,
              out.sio.turnOnCondition.clauseAOperator, out.sio.turnOnCondition.clauseAThreshold,
              out.sio.turnOnCondition.clauseBId, out.sio.turnOnCondition.clauseBOperator,
              out.sio.turnOnCondition.clauseBThreshold, out.sio.turnOnCondition.combiner, reason,
              sourceTypeById)) {
        return false;
      }
      if (!mapV3ConditionBlock(
              cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
              out.sio.turnOffCondition.clauseAId, out.sio.turnOffCondition.clauseAOperator,
              out.sio.turnOffCondition.clauseAThreshold, out.sio.turnOffCondition.clauseBId,
              out.sio.turnOffCondition.clauseBOperator, out.sio.turnOffCondition.clauseBThreshold,
              out.sio.turnOffCondition.combiner, reason, sourceTypeById)) {
        return false;
      }
    }
    return true;
  }

  if (expectedType == MathCard) {
    out.math.turnOnCondition.clauseAId = cardId;
    out.math.turnOnCondition.clauseAOperator = Op_AlwaysFalse;
    out.math.turnOnCondition.clauseAThreshold = 0U;
    out.math.turnOnCondition.clauseBId = cardId;
    out.math.turnOnCondition.clauseBOperator = Op_AlwaysFalse;
    out.math.turnOnCondition.clauseBThreshold = 0U;
    out.math.turnOnCondition.combiner = Combine_None;
    out.math.turnOffCondition.clauseAId = cardId;
    out.math.turnOffCondition.clauseAOperator = Op_AlwaysFalse;
    out.math.turnOffCondition.clauseAThreshold = 0U;
    out.math.turnOffCondition.clauseBId = cardId;
    out.math.turnOffCondition.clauseBOperator = Op_AlwaysFalse;
    out.math.turnOffCondition.clauseBThreshold = 0U;
    out.math.turnOffCondition.combiner = Combine_None;

    out.math.operation = 0U;
    out.math.fallbackValue = cfg["fallbackValue"] | 0U;
    JsonObjectConst standard = cfg["standard"].as<JsonObjectConst>();
    JsonObjectConst inputA = standard["inputA"].as<JsonObjectConst>();
    JsonObjectConst inputB = standard["inputB"].as<JsonObjectConst>();
    const char* op = standard["operator"] | "ADD";
    if (!parseMathOperator(op, out.math.operation)) {
      reason = "invalid MATH operator";
      return false;
    }
    out.math.inputA = inputA["value"] | 0U;
    out.math.inputB = inputB["value"] | 0U;
    out.math.inputMin = standard["inputMin"] | 0U;
    out.math.inputMax = standard["inputMax"] | 10000U;
    out.math.outputMin = standard["outputMin"] | 0U;
    out.math.outputMax = standard["outputMax"] | 10000U;
    out.math.smoothingFactorPct = standard["smoothingFactorPct"] | 100U;
    if (out.math.smoothingFactorPct > 100U) {
      reason = "MATH emaAlpha out of range";
      return false;
    }
    if (out.math.inputMin >= out.math.inputMax) {
      reason = "MATH input range invalid";
      return false;
    }
    if (cfg["turnOnCondition"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(
              cfg["turnOnCondition"].as<JsonObjectConst>(), totalCards, out.math.turnOnCondition.clauseAId,
              out.math.turnOnCondition.clauseAOperator, out.math.turnOnCondition.clauseAThreshold,
              out.math.turnOnCondition.clauseBId, out.math.turnOnCondition.clauseBOperator,
              out.math.turnOnCondition.clauseBThreshold, out.math.turnOnCondition.combiner, reason,
              sourceTypeById)) {
        return false;
      }
    }
    if (cfg["turnOffCondition"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(
              cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
              out.math.turnOffCondition.clauseAId, out.math.turnOffCondition.clauseAOperator,
              out.math.turnOffCondition.clauseAThreshold, out.math.turnOffCondition.clauseBId,
              out.math.turnOffCondition.clauseBOperator, out.math.turnOffCondition.clauseBThreshold,
              out.math.turnOffCondition.combiner, reason, sourceTypeById)) {
        return false;
      }
    }
    return true;
  }

  if (expectedType == RtcCard) {
    JsonObjectConst schedule = cfg["schedule"].as<JsonObjectConst>();
    if (schedule.isNull()) {
      reason = "RTC missing schedule object";
      return false;
    }
    if (!schedule["minute"].is<uint32_t>() && !schedule["minute"].is<int>()) {
      reason = "RTC minute is required";
      return false;
    }
    out.rtc.hasYear = schedule["hasYear"] | false;
    out.rtc.hasMonth = schedule["hasMonth"] | false;
    out.rtc.hasDay = schedule["hasDay"] | false;
    out.rtc.hasWeekday = schedule["hasWeekday"] | false;
    out.rtc.hasHour = schedule["hasHour"] | false;

    if (out.rtc.hasYear &&
        !(schedule["year"].is<uint32_t>() || schedule["year"].is<int>())) {
      reason = "RTC year required when hasYear is true";
      return false;
    }
    if (out.rtc.hasMonth &&
        !(schedule["month"].is<uint32_t>() || schedule["month"].is<int>())) {
      reason = "RTC month required when hasMonth is true";
      return false;
    }
    if (out.rtc.hasDay &&
        !(schedule["day"].is<uint32_t>() || schedule["day"].is<int>())) {
      reason = "RTC day required when hasDay is true";
      return false;
    }
    if (out.rtc.hasWeekday &&
        !(schedule["weekday"].is<uint32_t>() || schedule["weekday"].is<int>())) {
      reason = "RTC weekday required when hasWeekday is true";
      return false;
    }
    if (out.rtc.hasHour &&
        !(schedule["hour"].is<uint32_t>() || schedule["hour"].is<int>())) {
      reason = "RTC hour required when hasHour is true";
      return false;
    }

    out.rtc.year = schedule["year"] | 0;
    out.rtc.month = schedule["month"] | 0;
    out.rtc.day = schedule["day"] | 0;
    out.rtc.weekday = schedule["weekday"] | 0;
    out.rtc.hour = schedule["hour"] | 0;
    out.rtc.minute = schedule["minute"] | 0;
    out.rtc.triggerDurationMs = cfg["triggerDurationMs"] | 0U;
    return true;
  }

  reason = "unsupported card type";
  return false;
}


