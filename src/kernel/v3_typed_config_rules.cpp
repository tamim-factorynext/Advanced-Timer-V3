#include "kernel/v3_typed_config_rules.h"

#include <string>

namespace {
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

bool isNumericOp(logicOperator op) {
  return op == Op_GT || op == Op_GTE || op == Op_LT || op == Op_LTE ||
         op == Op_EQ || op == Op_NEQ;
}

bool isStateOp(logicOperator op) {
  return op == Op_LogicalTrue || op == Op_LogicalFalse ||
         op == Op_PhysicalOn || op == Op_PhysicalOff;
}

bool isTriggerOp(logicOperator op) {
  return op == Op_Triggered || op == Op_TriggerCleared;
}

bool isProcessOp(logicOperator op) {
  return op == Op_Running || op == Op_Finished || op == Op_Stopped;
}

bool isAlwaysOp(logicOperator op) {
  return op == Op_AlwaysTrue || op == Op_AlwaysFalse;
}

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
      if (!validateConditionBlock(card.di.set, card.cardId, "di.set")) {
        return false;
      }
      if (!validateConditionBlock(card.di.reset, card.cardId, "di.reset")) {
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
      if (!validateConditionBlock(card.dout.set, card.cardId, "do.set")) {
        return false;
      }
      if (!validateConditionBlock(card.dout.reset, card.cardId, "do.reset")) {
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
      if (card.ai.emaAlphaX100 > 100U) {
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
      if (!validateConditionBlock(card.sio.set, card.cardId, "sio.set")) {
        return false;
      }
      if (!validateConditionBlock(card.sio.reset, card.cardId, "sio.reset")) {
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
      if (card.math.emaAlphaX100 > 100U) {
        reason = "MATH emaAlpha out of range";
        return false;
      }
      if (!validateConditionBlock(card.math.set, card.cardId, "math.set")) {
        return false;
      }
      if (!validateConditionBlock(card.math.reset, card.cardId, "math.reset")) {
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
      if (card.rtc.hour > 23) {
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
