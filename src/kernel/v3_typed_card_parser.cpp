/*
File: src/kernel/v3_typed_card_parser.cpp
Purpose: Implements the v3 typed card parser module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
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
/**
 * @brief Resolves the expected legacy card type for a fixed card-id slot.
 * @details Slot boundaries are supplied by config and define a contiguous id
 * mapping for DI/DO/AI/SIO/MATH/RTC families.
 * @param id Card id being parsed.
 * @param doStart First DO card id.
 * @param aiStart First AI card id.
 * @param sioStart First SIO card id.
 * @param mathStart First MATH card id.
 * @param rtcStart First RTC card id.
 * @return The expected legacy card type for this id.
 * @par Used By
 * parseV3CardToTyped().
 */
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

/**
 * @brief Maps v3 mode token text into legacy runtime mode enum.
 * @details Conversion is type-aware so only valid mode tokens per family are
 * accepted.
 * @param type Parsed card type.
 * @param mode Mode token from JSON.
 * @param outMode Destination legacy mode.
 * @retval true Mode token is valid for the card type and was converted.
 * @retval false Mode token is invalid for the card type.
 * @par Used By
 * parseV3CardToTyped().
 */
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

/**
 * @brief Parses math operation token into compact operator code.
 * @param op Operation token string (ADD/SUB_SAT/MUL/DIV_SAFE).
 * @param outOp Destination operation id.
 * @retval true Operation token recognized.
 * @retval false Operation token invalid.
 * @par Used By
 * parseV3CardToTyped().
 */
bool parseMathOperator(const char* op, uint8_t& outOp) {
  if (op == nullptr) return false;
  if (std::strcmp(op, "ADD") == 0) return (outOp = 0U), true;
  if (std::strcmp(op, "SUB_SAT") == 0) return (outOp = 1U), true;
  if (std::strcmp(op, "MUL") == 0) return (outOp = 2U), true;
  if (std::strcmp(op, "DIV_SAFE") == 0) return (outOp = 3U), true;
  return false;
}

bool isNumericOperatorToken(const char* op) {
  if (op == nullptr) return false;
  return std::strcmp(op, "GT") == 0 || std::strcmp(op, "GTE") == 0 ||
         std::strcmp(op, "LT") == 0 || std::strcmp(op, "LTE") == 0 ||
         std::strcmp(op, "EQ") == 0 || std::strcmp(op, "NEQ") == 0;
}

/**
 * @brief Normalizes condition threshold JSON into uint32.
 * @details Accepts integer and bool forms and rejects negative or overflowing
 * values.
 * @param threshold JSON threshold value.
 * @param out Parsed numeric threshold.
 * @retval true Threshold converted successfully.
 * @retval false Threshold type/range unsupported.
 * @par Used By
 * mapV3ClauseToLegacyOperator().
 */
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

/**
 * @brief Converts a v3 clause triplet (field/operator/threshold) to legacy op.
 * @details Applies source-family field/operator compatibility rules first, then
 * maps state/edge/numeric forms into legacy logicOperator values.
 * @param sourceType Source card legacy type.
 * @param field Source field token.
 * @param op Operator token.
 * @param threshold Threshold JSON value.
 * @param ioThreshold Parsed numeric threshold output.
 * @param ok Conversion success flag.
 * @return Legacy operator for runtime condition evaluation.
 * @par Used By
 * mapV3ConditionBlock().
 */
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

/**
 * @brief Parses and maps a v3 condition block into legacy condition fields.
 * @details Supports `NONE`, `AND`, `OR` combiner and validates clause source
 * ids and clause compatibility with source card families.
 * @param block Condition block JSON object.
 * @param totalCards Total number of cards in config.
 * @param outAId Clause A source id.
 * @param outAOp Clause A operator.
 * @param outAThreshold Clause A threshold.
 * @param outBId Clause B source id.
 * @param outBOp Clause B operator.
 * @param outBThreshold Clause B threshold.
 * @param outCombine Clause combiner.
 * @param reason Human-readable parse error.
 * @param sourceTypeById Type lookup table by card id.
 * @retval true Block is valid and mapped.
 * @retval false Block invalid; `reason` set.
 * @par Used By
 * parseV3CardToTyped().
 */
bool mapV3ConditionBlock(JsonObjectConst block, uint8_t totalCards,
                         uint8_t& outAId, logicOperator& outAOp,
                         uint32_t& outAThreshold,
                         uint8_t& outAThresholdCardId,
                         bool& outAUseThresholdCard, uint8_t& outBId,
                         logicOperator& outBOp, uint32_t& outBThreshold,
                         uint8_t& outBThresholdCardId,
                         bool& outBUseThresholdCard,
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
  outAUseThresholdCard = a["useThresholdCard"] | false;
  outAThresholdCardId = a["thresholdCardId"] | outAId;
  if (outAUseThresholdCard) {
    if (!isNumericOperatorToken(aOperator)) {
      reason = "clauseA useThresholdCard requires numeric operator";
      return false;
    }
    if (outAThresholdCardId >= totalCards) {
      reason = "clauseA thresholdCardId out of range";
      return false;
    }
  }
  bool aOk = false;
  outAOp = mapV3ClauseToLegacyOperator(sourceTypeById[outAId], aField, aOperator,
                                       a["thresholdValue"], outAThreshold, aOk);
  if (!aOk) {
    reason = "invalid clauseA field/operator/threshold for source card type";
    return false;
  }

  outBId = outAId;
  outBOp = Op_AlwaysFalse;
  outBThreshold = 0;
  outBThresholdCardId = outAId;
  outBUseThresholdCard = false;
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
  outBUseThresholdCard = b["useThresholdCard"] | false;
  outBThresholdCardId = b["thresholdCardId"] | outBId;
  if (outBUseThresholdCard) {
    if (!isNumericOperatorToken(bOperator)) {
      reason = "clauseB useThresholdCard requires numeric operator";
      return false;
    }
    if (outBThresholdCardId >= totalCards) {
      reason = "clauseB thresholdCardId out of range";
      return false;
    }
  }
  bool bOk = false;
  outBOp = mapV3ClauseToLegacyOperator(sourceTypeById[outBId], bField, bOperator,
                                       b["thresholdValue"], outBThreshold, bOk);
  if (!bOk) {
    reason = "invalid clauseB field/operator/threshold for source card type";
    return false;
  }
  return true;
}

/**
 * @brief Converts legacy card type to typed-config family enum.
 * @param type Legacy card type.
 * @return Corresponding typed-card family.
 * @par Used By
 * parseV3CardToTyped().
 */
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

/**
 * @brief Parses one v3 card JSON object into strongly typed runtime config.
 * @details Performs slot/type consistency checks, mode mapping, per-family
 * field extraction, and condition mapping for family types that use logic
 * blocks.
 * @param v3Card Source card JSON object.
 * @param sourceTypeById Type lookup table by card id.
 * @param totalCards Total card count.
 * @param doStart First DO card id.
 * @param aiStart First AI card id.
 * @param sioStart First SIO card id.
 * @param mathStart First MATH card id.
 * @param rtcStart First RTC card id.
 * @param out Destination typed card config.
 * @param reason Human-readable parse error.
 * @retval true Card parsed successfully.
 * @retval false Card invalid; `reason` set.
 * @par Used By
 * normalizeConfigV3().
 */
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
                             out.di.turnOnCondition.clauseAThreshold,
                             out.di.turnOnCondition.clauseAThresholdCardId,
                             out.di.turnOnCondition.clauseAUseThresholdCard,
                             out.di.turnOnCondition.clauseBId,
                             out.di.turnOnCondition.clauseBOperator,
                             out.di.turnOnCondition.clauseBThreshold,
                             out.di.turnOnCondition.clauseBThresholdCardId,
                             out.di.turnOnCondition.clauseBUseThresholdCard,
                             out.di.turnOnCondition.combiner,
                             reason, sourceTypeById)) {
      return false;
    }
    if (!mapV3ConditionBlock(cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
                             out.di.turnOffCondition.clauseAId, out.di.turnOffCondition.clauseAOperator,
                             out.di.turnOffCondition.clauseAThreshold,
                             out.di.turnOffCondition.clauseAThresholdCardId,
                             out.di.turnOffCondition.clauseAUseThresholdCard,
                             out.di.turnOffCondition.clauseBId,
                             out.di.turnOffCondition.clauseBOperator,
                             out.di.turnOffCondition.clauseBThreshold,
                             out.di.turnOffCondition.clauseBThresholdCardId,
                             out.di.turnOffCondition.clauseBUseThresholdCard,
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
              out.dout.turnOnCondition.clauseAThresholdCardId,
              out.dout.turnOnCondition.clauseAUseThresholdCard,
              out.dout.turnOnCondition.clauseBId, out.dout.turnOnCondition.clauseBOperator,
              out.dout.turnOnCondition.clauseBThreshold,
              out.dout.turnOnCondition.clauseBThresholdCardId,
              out.dout.turnOnCondition.clauseBUseThresholdCard,
              out.dout.turnOnCondition.combiner, reason,
              sourceTypeById)) {
        return false;
      }
      if (!mapV3ConditionBlock(
              cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
              out.dout.turnOffCondition.clauseAId, out.dout.turnOffCondition.clauseAOperator,
              out.dout.turnOffCondition.clauseAThreshold,
              out.dout.turnOffCondition.clauseAThresholdCardId,
              out.dout.turnOffCondition.clauseAUseThresholdCard,
              out.dout.turnOffCondition.clauseBId,
              out.dout.turnOffCondition.clauseBOperator, out.dout.turnOffCondition.clauseBThreshold,
              out.dout.turnOffCondition.clauseBThresholdCardId,
              out.dout.turnOffCondition.clauseBUseThresholdCard,
              out.dout.turnOffCondition.combiner, reason, sourceTypeById)) {
        return false;
      }
    } else {
      if (!mapV3ConditionBlock(
              cfg["turnOnCondition"].as<JsonObjectConst>(), totalCards, out.sio.turnOnCondition.clauseAId,
              out.sio.turnOnCondition.clauseAOperator, out.sio.turnOnCondition.clauseAThreshold,
              out.sio.turnOnCondition.clauseAThresholdCardId,
              out.sio.turnOnCondition.clauseAUseThresholdCard,
              out.sio.turnOnCondition.clauseBId, out.sio.turnOnCondition.clauseBOperator,
              out.sio.turnOnCondition.clauseBThreshold,
              out.sio.turnOnCondition.clauseBThresholdCardId,
              out.sio.turnOnCondition.clauseBUseThresholdCard,
              out.sio.turnOnCondition.combiner, reason,
              sourceTypeById)) {
        return false;
      }
      if (!mapV3ConditionBlock(
              cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
              out.sio.turnOffCondition.clauseAId, out.sio.turnOffCondition.clauseAOperator,
              out.sio.turnOffCondition.clauseAThreshold,
              out.sio.turnOffCondition.clauseAThresholdCardId,
              out.sio.turnOffCondition.clauseAUseThresholdCard,
              out.sio.turnOffCondition.clauseBId,
              out.sio.turnOffCondition.clauseBOperator, out.sio.turnOffCondition.clauseBThreshold,
              out.sio.turnOffCondition.clauseBThresholdCardId,
              out.sio.turnOffCondition.clauseBUseThresholdCard,
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
    out.math.turnOnCondition.clauseAThresholdCardId = cardId;
    out.math.turnOnCondition.clauseAUseThresholdCard = false;
    out.math.turnOnCondition.clauseBThresholdCardId = cardId;
    out.math.turnOnCondition.clauseBUseThresholdCard = false;
    out.math.turnOnCondition.combiner = Combine_None;
    out.math.turnOffCondition.clauseAId = cardId;
    out.math.turnOffCondition.clauseAOperator = Op_AlwaysFalse;
    out.math.turnOffCondition.clauseAThreshold = 0U;
    out.math.turnOffCondition.clauseBId = cardId;
    out.math.turnOffCondition.clauseBOperator = Op_AlwaysFalse;
    out.math.turnOffCondition.clauseBThreshold = 0U;
    out.math.turnOffCondition.clauseAThresholdCardId = cardId;
    out.math.turnOffCondition.clauseAUseThresholdCard = false;
    out.math.turnOffCondition.clauseBThresholdCardId = cardId;
    out.math.turnOffCondition.clauseBUseThresholdCard = false;
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
              out.math.turnOnCondition.clauseAThresholdCardId,
              out.math.turnOnCondition.clauseAUseThresholdCard,
              out.math.turnOnCondition.clauseBId, out.math.turnOnCondition.clauseBOperator,
              out.math.turnOnCondition.clauseBThreshold,
              out.math.turnOnCondition.clauseBThresholdCardId,
              out.math.turnOnCondition.clauseBUseThresholdCard,
              out.math.turnOnCondition.combiner, reason,
              sourceTypeById)) {
        return false;
      }
    }
    if (cfg["turnOffCondition"].is<JsonObjectConst>()) {
      if (!mapV3ConditionBlock(
              cfg["turnOffCondition"].as<JsonObjectConst>(), totalCards,
              out.math.turnOffCondition.clauseAId, out.math.turnOffCondition.clauseAOperator,
              out.math.turnOffCondition.clauseAThreshold,
              out.math.turnOffCondition.clauseAThresholdCardId,
              out.math.turnOffCondition.clauseAUseThresholdCard,
              out.math.turnOffCondition.clauseBId,
              out.math.turnOffCondition.clauseBOperator, out.math.turnOffCondition.clauseBThreshold,
              out.math.turnOffCondition.clauseBThresholdCardId,
              out.math.turnOffCondition.clauseBUseThresholdCard,
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


