#include "kernel/v3_payload_rules.h"

#include <cstring>

#include "kernel/v3_condition_rules.h"

namespace {
logicCardType expectedTypeForId(uint8_t id, uint8_t doStart, uint8_t aiStart,
                                uint8_t sioStart, uint8_t mathStart,
                                uint8_t rtcStart) {
  if (id < doStart) return DigitalInput;
  if (id < aiStart) return DigitalOutput;
  if (id < sioStart) return AnalogInput;
  if (id < mathStart) return SoftIO;
  if (id < rtcStart) return MathCard;
  return RtcCard;
}

bool validateClause(JsonObjectConst clause, const logicCardType* sourceTypeById,
                    uint8_t totalCards, std::string& reason,
                    const char* clauseName) {
  if (!clause["source"].is<JsonObjectConst>()) {
    reason = std::string(clauseName) + ".source missing";
    return false;
  }
  JsonObjectConst source = clause["source"].as<JsonObjectConst>();
  uint8_t sourceId = source["cardId"] | 255;
  if (sourceId >= totalCards) {
    reason = std::string(clauseName) + ".source.cardId out of range";
    return false;
  }
  const char* field = source["field"] | "";
  const char* op = clause["operator"] | "";
  if (!isV3FieldAllowedForSourceType(sourceTypeById[sourceId], field)) {
    reason = std::string(clauseName) + ".source.field not allowed for source type";
    return false;
  }
  if (!isV3OperatorAllowedForField(field, op)) {
    reason = std::string(clauseName) + ".operator not allowed for field";
    return false;
  }
  if (std::strcmp(field, "missionState") == 0) {
    const char* threshold = clause["threshold"] | "";
    if (!(std::strcmp(threshold, "IDLE") == 0 ||
          std::strcmp(threshold, "ACTIVE") == 0 ||
          std::strcmp(threshold, "FINISHED") == 0)) {
      reason = std::string(clauseName) + ".threshold invalid missionState";
      return false;
    }
  }
  return true;
}

bool validateConditionBlock(JsonObjectConst block, const logicCardType* sourceTypeById,
                            uint8_t totalCards, std::string& reason,
                            const char* blockName) {
  if (block.isNull()) return true;
  const char* combiner = block["combiner"] | "NONE";
  bool needsClauseB = false;
  if (std::strcmp(combiner, "NONE") == 0) {
    needsClauseB = false;
  } else if (std::strcmp(combiner, "AND") == 0 ||
             std::strcmp(combiner, "OR") == 0) {
    needsClauseB = true;
  } else {
    reason = std::string(blockName) + ".combiner invalid";
    return false;
  }

  if (!block["clauseA"].is<JsonObjectConst>()) {
    reason = std::string(blockName) + ".clauseA missing";
    return false;
  }
  if (!validateClause(block["clauseA"].as<JsonObjectConst>(), sourceTypeById,
                      totalCards, reason, "clauseA")) {
    return false;
  }
  if (needsClauseB) {
    if (!block["clauseB"].is<JsonObjectConst>()) {
      reason = std::string(blockName) + ".clauseB missing";
      return false;
    }
    if (!validateClause(block["clauseB"].as<JsonObjectConst>(), sourceTypeById,
                        totalCards, reason, "clauseB")) {
      return false;
    }
  }
  return true;
}
}  // namespace

bool validateV3PayloadConditionSources(JsonArrayConst cards, uint8_t totalCards,
                                       uint8_t doStart, uint8_t aiStart,
                                       uint8_t sioStart, uint8_t mathStart,
                                       uint8_t rtcStart, std::string& reason) {
  if (cards.size() == 0) {
    reason = "cards array empty";
    return false;
  }

  bool seen[255] = {};
  logicCardType sourceTypeById[255] = {};

  if (totalCards > 255) {
    reason = "totalCards exceeds validator capacity";
    return false;
  }

  for (JsonVariantConst v : cards) {
    if (!v.is<JsonObjectConst>()) {
      reason = "cards[] item is not object";
      return false;
    }
    JsonObjectConst card = v.as<JsonObjectConst>();
    uint8_t id = card["cardId"] | 255;
    if (id >= totalCards) {
      reason = "cardId out of range";
      return false;
    }
    if (seen[id]) {
      reason = "duplicate cardId";
      return false;
    }
    seen[id] = true;
    logicCardType parsed = DigitalInput;
    if (!parseV3CardTypeToken(card["cardType"] | "", parsed)) {
      reason = "invalid cardType token";
      return false;
    }
    logicCardType expected =
        expectedTypeForId(id, doStart, aiStart, sioStart, mathStart, rtcStart);
    if (parsed != expected) {
      reason = "cardType does not match family slot";
      return false;
    }
    sourceTypeById[id] = parsed;
  }

  for (JsonVariantConst v : cards) {
    JsonObjectConst card = v.as<JsonObjectConst>();
    logicCardType type = DigitalInput;
    parseV3CardTypeToken(card["cardType"] | "", type);
    JsonObjectConst cfg = card["config"].as<JsonObjectConst>();
    if (cfg.isNull()) continue;
    bool usesConditions = (type == DigitalInput || type == DigitalOutput ||
                           type == SoftIO || type == MathCard);
    if (!usesConditions) continue;

    if (cfg["set"].is<JsonObjectConst>()) {
      if (!validateConditionBlock(cfg["set"].as<JsonObjectConst>(), sourceTypeById,
                                  totalCards, reason, "set")) {
        return false;
      }
    }
    if (cfg["reset"].is<JsonObjectConst>()) {
      if (!validateConditionBlock(cfg["reset"].as<JsonObjectConst>(),
                                  sourceTypeById, totalCards, reason, "reset")) {
        return false;
      }
    }
  }

  reason.clear();
  return true;
}

