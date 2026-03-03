/*
File: src/kernel/v3_typed_config_rules.cpp
Purpose: Implements the v3 typed config rules module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/v3_typed_config_rules.h
- src/storage/v3_config_service.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_typed_config_rules.h"

#include <string>

namespace {
/**
 * @brief Resolves expected card family for a fixed card-id slot.
 * @details Uses configured family start offsets to classify each contiguous id
 * range.
 * @param id Card id.
 * @param doStart First DO id.
 * @param aiStart First AI id.
 * @param sioStart First SIO id.
 * @param mathStart First MATH id.
 * @param rtcStart First RTC id.
 * @return Expected typed-card family for the slot.
 * @par Used By
 * validateTypedCardConfigs().
 */
V3CardFamily familyForId(uint8_t id, uint8_t doStart, uint8_t aiStart,
                         uint8_t sioStart, uint8_t mathStart,
                         uint8_t rtcStart) {
  if (id < doStart) return V3CardFamily::DI;
  if (id < aiStart) return V3CardFamily::DO;
  if (id < sioStart) return V3CardFamily::AI;
  if (id < mathStart) return V3CardFamily::SIO;
  if (id < rtcStart) return V3CardFamily::MATH;
  return V3CardFamily::RTC;
}

/**
 * @brief Checks whether an operator is a numeric comparison operator.
 * @par Used By
 * isOperatorAllowedForFamily().
 */
bool isNumericOp(logicOperator op) {
  return op == Op_GT || op == Op_GTE || op == Op_LT || op == Op_LTE ||
         op == Op_EQ || op == Op_NEQ;
}

bool familyHasNumericLiveValue(V3CardFamily family) {
  return family != V3CardFamily::RTC;
}

/**
 * @brief Checks whether an operator is a logical/physical state operator.
 * @par Used By
 * isOperatorAllowedForFamily().
 */
bool isStateOp(logicOperator op) {
  return op == Op_LogicalTrue || op == Op_LogicalFalse ||
         op == Op_PhysicalOn || op == Op_PhysicalOff;
}

/**
 * @brief Checks whether an operator is a trigger-edge operator.
 * @par Used By
 * isOperatorAllowedForFamily().
 */
bool isTriggerOp(logicOperator op) {
  return op == Op_Triggered || op == Op_TriggerCleared;
}

/**
 * @brief Checks whether an operator is a process-state operator.
 * @par Used By
 * isOperatorAllowedForFamily().
 */
bool isProcessOp(logicOperator op) {
  return op == Op_Running || op == Op_Finished || op == Op_Stopped;
}

/**
 * @brief Checks whether an operator is an unconditional always operator.
 * @par Used By
 * isOperatorAllowedForFamily().
 */
bool isAlwaysOp(logicOperator op) {
  return op == Op_AlwaysTrue || op == Op_AlwaysFalse;
}

/**
 * @brief Validates operator compatibility with a source card family.
 * @details Enforces family-specific rule set so condition clauses only use
 * operators that runtime semantics support for that source family.
 * @param sourceFamily Clause source family.
 * @param op Clause operator.
 * @retval true Operator is allowed for this source family.
 * @retval false Operator is not allowed.
 * @par Used By
 * validateTypedCardConfigs().
 */
bool isOperatorAllowedForFamily(V3CardFamily sourceFamily, logicOperator op) {
  if (isAlwaysOp(op)) return true;
  if (sourceFamily == V3CardFamily::AI) {
    return isNumericOp(op);
  }
  if (sourceFamily == V3CardFamily::MATH) {
    return isNumericOp(op) || isTriggerOp(op);
  }
  if (sourceFamily == V3CardFamily::DI || sourceFamily == V3CardFamily::RTC) {
    return isStateOp(op) || isTriggerOp(op) || isNumericOp(op);
  }
  if (sourceFamily == V3CardFamily::DO || sourceFamily == V3CardFamily::SIO) {
    return isStateOp(op) || isTriggerOp(op) || isNumericOp(op) ||
           isProcessOp(op);
  }
  return false;
}
}  // namespace

/**
 * @brief Validates normalized typed-card configuration for runtime safety.
 * @details Confirms card ordering, slot-family alignment, family-specific field
 * ranges, mode values, and condition operator compatibility.
 * @param cards Typed card config array.
 * @param count Number of cards.
 * @param doStart First DO id.
 * @param aiStart First AI id.
 * @param sioStart First SIO id.
 * @param mathStart First MATH id.
 * @param rtcStart First RTC id.
 * @param reason Human-readable validation error on failure.
 * @retval true Configuration is valid.
 * @retval false Configuration invalid; `reason` set.
 * @par Used By
 * parseAndValidateConfigV3().
 */
bool validateTypedCardConfigs(const V3CardConfig* cards, uint8_t count,
                              uint8_t doStart, uint8_t aiStart,
                              uint8_t sioStart, uint8_t mathStart,
                              uint8_t rtcStart, std::string& reason) {
  if (cards == nullptr) {
    reason = "typed cards missing";
    return false;
  }
  if (count == 0) {
    reason = "typed cards empty";
    return false;
  }

  auto validateConditionBlock = [&](const V3ConditionBlock& block,
                                    uint8_t ownerId,
                                    const char* label) -> bool {
    if (block.clauseAId >= count || block.clauseBId >= count) {
      reason = std::string(label) + " source id out of range for card " +
               std::to_string(ownerId);
      return false;
    }
    if (!isOperatorAllowedForFamily(cards[block.clauseAId].family,
                                    block.clauseAOperator)) {
      reason = std::string(label) +
               " clauseA operator not allowed for source family";
      return false;
    }
    if (block.clauseAUseThresholdCard) {
      if (!isNumericOp(block.clauseAOperator)) {
        reason = std::string(label) +
                 " clauseA threshold card requires numeric operator";
        return false;
      }
      if (block.clauseAThresholdCardId >= count) {
        reason = std::string(label) + " clauseA threshold card id out of range";
        return false;
      }
      if (block.clauseAThresholdCardId == block.clauseAId) {
        reason = std::string(label) + " clauseA threshold card self-reference";
        return false;
      }
      if (!familyHasNumericLiveValue(cards[block.clauseAThresholdCardId].family)) {
        reason = std::string(label) +
                 " clauseA threshold card has no numeric liveValue";
        return false;
      }
    }
    if (block.combiner != Combine_None && block.combiner != Combine_AND &&
        block.combiner != Combine_OR) {
      reason = std::string(label) + " combiner invalid";
      return false;
    }
    if (block.combiner == Combine_None) return true;
    if (!isOperatorAllowedForFamily(cards[block.clauseBId].family,
                                    block.clauseBOperator)) {
      reason = std::string(label) +
               " clauseB operator not allowed for source family";
      return false;
    }
    if (block.clauseBUseThresholdCard) {
      if (!isNumericOp(block.clauseBOperator)) {
        reason = std::string(label) +
                 " clauseB threshold card requires numeric operator";
        return false;
      }
      if (block.clauseBThresholdCardId >= count) {
        reason = std::string(label) + " clauseB threshold card id out of range";
        return false;
      }
      if (block.clauseBThresholdCardId == block.clauseBId) {
        reason = std::string(label) + " clauseB threshold card self-reference";
        return false;
      }
      if (!familyHasNumericLiveValue(cards[block.clauseBThresholdCardId].family)) {
        reason = std::string(label) +
                 " clauseB threshold card has no numeric liveValue";
        return false;
      }
    }
    return true;
  };

  for (uint8_t i = 0; i < count; ++i) {
    const V3CardConfig& card = cards[i];
    if (card.cardId != i) {
      reason = "typed cardId ordering mismatch";
      return false;
    }

    const V3CardFamily expectedFamily =
        familyForId(i, doStart, aiStart, sioStart, mathStart, rtcStart);
    if (card.family != expectedFamily) {
      reason = "typed card family does not match fixed family slot";
      return false;
    }

    if (card.family == V3CardFamily::DI) {
      if (card.di.edgeMode != Mode_DI_Rising &&
          card.di.edgeMode != Mode_DI_Falling &&
          card.di.edgeMode != Mode_DI_Change) {
        reason = "DI edge mode invalid";
        return false;
      }
      if (!validateConditionBlock(card.di.turnOnCondition, card.cardId,
                                  "di.turnOnCondition")) {
        return false;
      }
      if (!validateConditionBlock(card.di.turnOffCondition, card.cardId,
                                  "di.turnOffCondition")) {
        return false;
      }
      continue;
    }

    if (card.family == V3CardFamily::DO) {
      if (card.dout.mode != Mode_DO_Normal &&
          card.dout.mode != Mode_DO_Immediate &&
          card.dout.mode != Mode_DO_Gated) {
        reason = "DO mode invalid";
        return false;
      }
      if (!validateConditionBlock(card.dout.turnOnCondition, card.cardId,
                                  "do.turnOnCondition")) {
        return false;
      }
      if (!validateConditionBlock(card.dout.turnOffCondition, card.cardId,
                                  "do.turnOffCondition")) {
        return false;
      }
      continue;
    }

    if (card.family == V3CardFamily::AI) {
      if (card.ai.inputMin > card.ai.inputMax) {
        reason = "AI input range invalid";
        return false;
      }
      if (card.ai.outputMin > card.ai.outputMax) {
        reason = "AI output range invalid";
        return false;
      }
      if (card.ai.smoothingFactorPct > 100U) {
        reason = "AI emaAlpha out of range";
        return false;
      }
      continue;
    }

    if (card.family == V3CardFamily::SIO) {
      if (card.sio.mode != Mode_DO_Normal &&
          card.sio.mode != Mode_DO_Immediate &&
          card.sio.mode != Mode_DO_Gated) {
        reason = "SIO mode invalid";
        return false;
      }
      if (!validateConditionBlock(card.sio.turnOnCondition, card.cardId,
                                  "sio.turnOnCondition")) {
        return false;
      }
      if (!validateConditionBlock(card.sio.turnOffCondition, card.cardId,
                                  "sio.turnOffCondition")) {
        return false;
      }
      continue;
    }

    if (card.family == V3CardFamily::MATH) {
      if (card.math.operation > 3U) {
        reason = "MATH operator invalid";
        return false;
      }
      if (card.math.inputMin >= card.math.inputMax) {
        reason = "MATH input range invalid";
        return false;
      }
      if (card.math.smoothingFactorPct > 100U) {
        reason = "MATH emaAlpha out of range";
        return false;
      }
      if (!validateConditionBlock(card.math.turnOnCondition, card.cardId,
                                  "math.turnOnCondition")) {
        return false;
      }
      if (!validateConditionBlock(card.math.turnOffCondition, card.cardId,
                                  "math.turnOffCondition")) {
        return false;
      }
      continue;
    }

    if (card.family == V3CardFamily::RTC) {
      if (card.rtc.hasMonth && (card.rtc.month < 1 || card.rtc.month > 12)) {
        reason = "RTC month out of range";
        return false;
      }
      if (card.rtc.hasDay && (card.rtc.day < 1 || card.rtc.day > 31)) {
        reason = "RTC day out of range";
        return false;
      }
      if (card.rtc.hasWeekday && card.rtc.weekday > 6) {
        reason = "RTC weekday out of range";
        return false;
      }
      if (card.rtc.hasHour && card.rtc.hour > 23) {
        reason = "RTC hour out of range";
        return false;
      }
      if (card.rtc.minute > 59) {
        reason = "RTC minute out of range";
        return false;
      }
      continue;
    }
  }

  reason = "";
  return true;
}

