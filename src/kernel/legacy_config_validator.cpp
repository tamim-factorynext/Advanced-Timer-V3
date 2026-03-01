#include "kernel/legacy_config_validator.h"

#include <cstring>

#include "kernel/enum_codec.h"

bool validateLegacyConfigCardsArray(JsonArrayConst array, uint8_t totalCards,
                                    uint8_t doStart, uint8_t aiStart,
                                    uint8_t sioStart, uint8_t mathStart,
                                    uint8_t rtcStart, String& reason) {
  if (array.size() != totalCards) {
    reason = "cards size mismatch";
    return false;
  }

  auto cardTypeFromString = [](const char* s) -> logicCardType {
    return parseOrDefault(s, DigitalInput);
  };
  auto isModeAllowed = [](logicCardType type, const char* mode) -> bool {
    if (mode == nullptr) return false;
    if (type == DigitalInput) {
      return strcmp(mode, "Mode_DI_Rising") == 0 ||
             strcmp(mode, "Mode_DI_Falling") == 0 ||
             strcmp(mode, "Mode_DI_Change") == 0;
    }
    if (type == AnalogInput) return strcmp(mode, "Mode_AI_Continuous") == 0;
    if (type == DigitalOutput || type == SoftIO) {
      return strcmp(mode, "Mode_DO_Normal") == 0 ||
             strcmp(mode, "Mode_DO_Immediate") == 0 ||
             strcmp(mode, "Mode_DO_Gated") == 0;
    }
    if (type == MathCard || type == RtcCard) {
      return strcmp(mode, "Mode_None") == 0;
    }
    return false;
  };
  auto isAlwaysOp = [](const char* op) -> bool {
    return op != nullptr && (strcmp(op, "Op_AlwaysTrue") == 0 ||
                             strcmp(op, "Op_AlwaysFalse") == 0);
  };
  auto isNumericOp = [](const char* op) -> bool {
    return op != nullptr &&
           (strcmp(op, "Op_GT") == 0 || strcmp(op, "Op_LT") == 0 ||
            strcmp(op, "Op_EQ") == 0 || strcmp(op, "Op_NEQ") == 0 ||
            strcmp(op, "Op_GTE") == 0 || strcmp(op, "Op_LTE") == 0);
  };
  auto isStateOp = [](const char* op) -> bool {
    return op != nullptr && (strcmp(op, "Op_LogicalTrue") == 0 ||
                             strcmp(op, "Op_LogicalFalse") == 0 ||
                             strcmp(op, "Op_PhysicalOn") == 0 ||
                             strcmp(op, "Op_PhysicalOff") == 0);
  };
  auto isTriggerOp = [](const char* op) -> bool {
    return op != nullptr && (strcmp(op, "Op_Triggered") == 0 ||
                             strcmp(op, "Op_TriggerCleared") == 0);
  };
  auto isProcessOp = [](const char* op) -> bool {
    return op != nullptr &&
           (strcmp(op, "Op_Running") == 0 || strcmp(op, "Op_Finished") == 0 ||
            strcmp(op, "Op_Stopped") == 0);
  };
  auto isOperatorAllowedForTarget = [&](logicCardType targetType,
                                        const char* op) -> bool {
    if (isAlwaysOp(op)) return true;
    if (targetType == AnalogInput) return isNumericOp(op);
    if (targetType == DigitalInput)
      return isStateOp(op) || isTriggerOp(op) || isNumericOp(op);
    if (targetType == DigitalOutput || targetType == SoftIO) {
      return isStateOp(op) || isTriggerOp(op) || isNumericOp(op) ||
             isProcessOp(op);
    }
    if (targetType == MathCard) return isNumericOp(op);
    if (targetType == RtcCard) {
      return isStateOp(op) || isTriggerOp(op) || isNumericOp(op);
    }
    return false;
  };
  auto isNonNegativeNumber = [](JsonVariantConst v) -> bool {
    if (v.is<uint64_t>()) return true;
    if (v.is<int64_t>()) return v.as<int64_t>() >= 0;
    if (v.is<double>()) return v.as<double>() >= 0.0;
    if (v.is<float>()) return v.as<float>() >= 0.0f;
    return false;
  };
  auto ensureNonNegativeField = [&](JsonObjectConst card, const char* fieldName,
                                    const char* label) -> bool {
    JsonVariantConst v = card[fieldName];
    if (!isNonNegativeNumber(v)) {
      reason = String(label) + " must be non-negative";
      return false;
    }
    return true;
  };
  auto rejectFieldIfPresent = [&](JsonObjectConst card, const char* fieldName,
                                  const char* label) -> bool {
    if (!card[fieldName].isUnbound()) {
      reason = String(label) + " is runtime-only and not allowed in config";
      return false;
    }
    return true;
  };

  bool seenId[255] = {};
  logicCardType typeById[255] = {};
  bool typeKnown[255] = {};
  if (totalCards > 255) {
    reason = "total cards exceeds supported validator capacity";
    return false;
  }
  for (uint8_t i = 0; i < totalCards; ++i) {
    JsonVariantConst item = array[i];
    if (!item.is<JsonObjectConst>()) {
      reason = "cards[] item is not object";
      return false;
    }
    JsonObjectConst card = item.as<JsonObjectConst>();
    uint8_t id = card["id"] | 255;
    if (id >= totalCards) {
      reason = "card id out of range";
      return false;
    }
    if (seenId[id]) {
      reason = "duplicate card id";
      return false;
    }
    seenId[id] = true;
    typeById[id] = cardTypeFromString(card["type"] | "DigitalInput");
    typeKnown[id] = true;
    logicCardType expectedType = DigitalInput;
    if (id < doStart) {
      expectedType = DigitalInput;
    } else if (id < aiStart) {
      expectedType = DigitalOutput;
    } else if (id < sioStart) {
      expectedType = AnalogInput;
    } else if (id < mathStart) {
      expectedType = SoftIO;
    } else if (id < rtcStart) {
      expectedType = MathCard;
    } else {
      expectedType = RtcCard;
    }
    if (typeById[id] != expectedType) {
      reason = "card type does not match fixed family slot";
      return false;
    }

    uint8_t setAId = card["setA_ID"] | 255;
    uint8_t setBId = card["setB_ID"] | 255;
    uint8_t resetAId = card["resetA_ID"] | 255;
    uint8_t resetBId = card["resetB_ID"] | 255;
    if (setAId >= totalCards || setBId >= totalCards || resetAId >= totalCards ||
        resetBId >= totalCards) {
      reason = "set/reset reference id out of range";
      return false;
    }
  }

  for (uint8_t i = 0; i < totalCards; ++i) {
    JsonObjectConst card = array[i].as<JsonObjectConst>();
    uint8_t id = card["id"] | 255;
    if (id >= totalCards || !typeKnown[id]) {
      reason = "card id/type map error";
      return false;
    }

    const char* mode = card["mode"] | "";
    if (!isModeAllowed(typeById[id], mode)) {
      reason = "mode not valid for card type (id=" + String(id) +
               ", type=" + String(toString(typeById[id])) +
               ", mode=" + String(mode) + ")";
      return false;
    }

    if (!ensureNonNegativeField(card, "hwPin", "hwPin")) return false;
    if (!ensureNonNegativeField(card, "setting1", "setting1")) return false;
    if (!ensureNonNegativeField(card, "setting2", "setting2")) return false;
    if (!ensureNonNegativeField(card, "setting3", "setting3")) return false;
    if (!rejectFieldIfPresent(card, "logicalState", "logicalState")) return false;
    if (!rejectFieldIfPresent(card, "physicalState", "physicalState")) return false;
    if (!rejectFieldIfPresent(card, "triggerFlag", "triggerFlag")) return false;
    if (!rejectFieldIfPresent(card, "currentValue", "currentValue")) return false;
    if (!rejectFieldIfPresent(card, "repeatCounter", "repeatCounter")) return false;
    if (!rejectFieldIfPresent(card, "state", "state")) return false;

    if (typeById[id] == AnalogInput || typeById[id] == MathCard) {
      if (!ensureNonNegativeField(card, "startOnMs", "startOnMs")) return false;
      if (!ensureNonNegativeField(card, "startOffMs", "startOffMs")) return false;
    } else {
      if (!card["startOnMs"].isUnbound()) {
        reason = "startOnMs is only allowed for AI/MATH cards";
        return false;
      }
      if (!card["startOffMs"].isUnbound()) {
        reason = "startOffMs is only allowed for AI/MATH cards";
        return false;
      }
    }
    if (!ensureNonNegativeField(card, "setA_Threshold", "setA_Threshold"))
      return false;
    if (!ensureNonNegativeField(card, "setB_Threshold", "setB_Threshold"))
      return false;
    if (!ensureNonNegativeField(card, "resetA_Threshold", "resetA_Threshold"))
      return false;
    if (!ensureNonNegativeField(card, "resetB_Threshold", "resetB_Threshold"))
      return false;

    if (typeById[id] == AnalogInput) {
      const double alpha = card["setting3"] | 0.0;
      if (alpha < 0.0 || alpha > 1.0) {
        reason = "AI setting3 alpha out of range (0..1)";
        return false;
      }
    }

    if (typeById[id] == RtcCard) {
      const char* setAOp = card["setA_Operator"] | "";
      const char* setBOp = card["setB_Operator"] | "";
      const char* resetAOp = card["resetA_Operator"] | "";
      const char* resetBOp = card["resetB_Operator"] | "";
      const char* setCombine = card["setCombine"] | "";
      const char* resetCombine = card["resetCombine"] | "";
      const bool rtcSetResetDisabled =
          strcmp(setAOp, "Op_AlwaysFalse") == 0 &&
          strcmp(setBOp, "Op_AlwaysFalse") == 0 &&
          strcmp(resetAOp, "Op_AlwaysFalse") == 0 &&
          strcmp(resetBOp, "Op_AlwaysFalse") == 0 &&
          strcmp(setCombine, "Combine_None") == 0 &&
          strcmp(resetCombine, "Combine_None") == 0;
      if (!rtcSetResetDisabled) {
        reason = "RTC set/reset is unsupported";
        return false;
      }
    }

    uint8_t setAId = card["setA_ID"] | 255;
    uint8_t setBId = card["setB_ID"] | 255;
    uint8_t resetAId = card["resetA_ID"] | 255;
    uint8_t resetBId = card["resetB_ID"] | 255;
    const char* setAOp = card["setA_Operator"] | "";
    const char* setBOp = card["setB_Operator"] | "";
    const char* resetAOp = card["resetA_Operator"] | "";
    const char* resetBOp = card["resetB_Operator"] | "";

    if (!isOperatorAllowedForTarget(typeById[setAId], setAOp) ||
        !isOperatorAllowedForTarget(typeById[setBId], setBOp) ||
        !isOperatorAllowedForTarget(typeById[resetAId], resetAOp) ||
        !isOperatorAllowedForTarget(typeById[resetBId], resetBOp)) {
      reason = "operator not valid for referenced card type";
      return false;
    }
  }

  reason = "";
  return true;
}
