#include "storage/v3_config_decoder.h"

#include <string.h>

namespace v3::storage {

namespace {

void initDefaultConditionBlock(v3::storage::ConditionBlock& block, uint8_t selfCardId,
                               bool enabled) {
  (void)enabled;
  block.combiner = v3::storage::ConditionCombiner::None;
  block.clauseA.sourceCardId = selfCardId;
  block.clauseA.op = v3::storage::ConditionOperator::AlwaysFalse;
  block.clauseA.threshold = 0;
  block.clauseB.sourceCardId = selfCardId;
  block.clauseB.op = v3::storage::ConditionOperator::AlwaysFalse;
  block.clauseB.threshold = 0;
}

bool parseConditionOperator(const char* token, v3::storage::ConditionOperator& out) {
  if (token == nullptr) return false;
  if (strcmp(token, "ALWAYS_TRUE") == 0 || strcmp(token, "Op_AlwaysTrue") == 0) {
    out = v3::storage::ConditionOperator::AlwaysTrue;
    return true;
  }
  if (strcmp(token, "ALWAYS_FALSE") == 0 || strcmp(token, "Op_AlwaysFalse") == 0) {
    out = v3::storage::ConditionOperator::AlwaysFalse;
    return true;
  }
  if (strcmp(token, "LOGICAL_TRUE") == 0 || strcmp(token, "Op_LogicalTrue") == 0) {
    out = v3::storage::ConditionOperator::LogicalTrue;
    return true;
  }
  if (strcmp(token, "LOGICAL_FALSE") == 0 ||
      strcmp(token, "Op_LogicalFalse") == 0) {
    out = v3::storage::ConditionOperator::LogicalFalse;
    return true;
  }
  if (strcmp(token, "PHYSICAL_ON") == 0 || strcmp(token, "Op_PhysicalOn") == 0) {
    out = v3::storage::ConditionOperator::PhysicalOn;
    return true;
  }
  if (strcmp(token, "PHYSICAL_OFF") == 0 || strcmp(token, "Op_PhysicalOff") == 0) {
    out = v3::storage::ConditionOperator::PhysicalOff;
    return true;
  }
  if (strcmp(token, "TRIGGERED") == 0 || strcmp(token, "Op_Triggered") == 0) {
    out = v3::storage::ConditionOperator::Triggered;
    return true;
  }
  if (strcmp(token, "TRIGGER_CLEARED") == 0 ||
      strcmp(token, "Op_TriggerCleared") == 0) {
    out = v3::storage::ConditionOperator::TriggerCleared;
    return true;
  }
  if (strcmp(token, "GT") == 0 || strcmp(token, "Op_GT") == 0) {
    out = v3::storage::ConditionOperator::GT;
    return true;
  }
  if (strcmp(token, "LT") == 0 || strcmp(token, "Op_LT") == 0) {
    out = v3::storage::ConditionOperator::LT;
    return true;
  }
  if (strcmp(token, "EQ") == 0 || strcmp(token, "Op_EQ") == 0) {
    out = v3::storage::ConditionOperator::EQ;
    return true;
  }
  if (strcmp(token, "NEQ") == 0 || strcmp(token, "Op_NEQ") == 0) {
    out = v3::storage::ConditionOperator::NEQ;
    return true;
  }
  if (strcmp(token, "GTE") == 0 || strcmp(token, "Op_GTE") == 0) {
    out = v3::storage::ConditionOperator::GTE;
    return true;
  }
  if (strcmp(token, "LTE") == 0 || strcmp(token, "Op_LTE") == 0) {
    out = v3::storage::ConditionOperator::LTE;
    return true;
  }
  if (strcmp(token, "RUNNING") == 0 || strcmp(token, "Op_Running") == 0) {
    out = v3::storage::ConditionOperator::Running;
    return true;
  }
  if (strcmp(token, "FINISHED") == 0 || strcmp(token, "Op_Finished") == 0) {
    out = v3::storage::ConditionOperator::Finished;
    return true;
  }
  if (strcmp(token, "STOPPED") == 0 || strcmp(token, "Op_Stopped") == 0) {
    out = v3::storage::ConditionOperator::Stopped;
    return true;
  }
  return false;
}

bool parseConditionCombiner(const char* token, v3::storage::ConditionCombiner& out) {
  if (token == nullptr) return false;
  if (strcmp(token, "NONE") == 0) {
    out = v3::storage::ConditionCombiner::None;
    return true;
  }
  if (strcmp(token, "AND") == 0) {
    out = v3::storage::ConditionCombiner::And;
    return true;
  }
  if (strcmp(token, "OR") == 0) {
    out = v3::storage::ConditionCombiner::Or;
    return true;
  }
  return false;
}

bool parseConditionClause(JsonObjectConst obj, v3::storage::ConditionClause& outClause) {
  if (!obj["sourceCardId"].is<uint8_t>()) return false;
  if (!obj["threshold"].is<uint32_t>()) return false;
  const char* opToken = obj["operator"].as<const char*>();
  v3::storage::ConditionOperator op = v3::storage::ConditionOperator::AlwaysFalse;
  if (!parseConditionOperator(opToken, op)) return false;
  outClause.sourceCardId = obj["sourceCardId"].as<uint8_t>();
  outClause.op = op;
  outClause.threshold = obj["threshold"].as<uint32_t>();
  return true;
}

bool parseConditionBlockOptional(JsonObjectConst obj, v3::storage::ConditionBlock& outBlock) {
  if (obj.isNull()) return false;
  if (!obj["clauseA"].is<JsonObjectConst>() || !obj["combiner"].is<const char*>()) {
    return false;
  }
  v3::storage::ConditionCombiner combiner = v3::storage::ConditionCombiner::None;
  if (!parseConditionCombiner(obj["combiner"].as<const char*>(), combiner)) return false;
  if (!parseConditionClause(obj["clauseA"].as<JsonObjectConst>(), outBlock.clauseA)) {
    return false;
  }
  outBlock.combiner = combiner;

  if (combiner == v3::storage::ConditionCombiner::None) {
    outBlock.clauseB = outBlock.clauseA;
    outBlock.clauseB.op = v3::storage::ConditionOperator::AlwaysFalse;
    outBlock.clauseB.threshold = 0;
    return true;
  }

  if (!obj["clauseB"].is<JsonObjectConst>()) return false;
  if (!parseConditionClause(obj["clauseB"].as<JsonObjectConst>(), outBlock.clauseB)) {
    return false;
  }
  return true;
}

bool parseStringField(JsonObjectConst obj, const char* key, char* out,
                      size_t outSize) {
  if (!obj[key].is<const char*>()) return false;
  const char* value = obj[key].as<const char*>();
  if (value == nullptr) return false;
  const size_t len = strlen(value);
  if (len == 0 || len >= outSize) return false;
  memcpy(out, value, len);
  out[len] = '\0';
  return true;
}

bool parseWiFiCredential(JsonObjectConst obj, WiFiCredential& outCredential) {
  if (obj.isNull()) return false;
  if (!parseStringField(obj, "ssid", outCredential.ssid,
                        sizeof(outCredential.ssid))) {
    return false;
  }
  if (!parseStringField(obj, "password", outCredential.password,
                        sizeof(outCredential.password))) {
    return false;
  }
  if (!obj["timeoutSec"].is<uint32_t>()) return false;
  if (!obj["editable"].is<bool>()) return false;
  outCredential.timeoutSec = obj["timeoutSec"].as<uint32_t>();
  outCredential.editable = obj["editable"].as<bool>();
  return true;
}

bool parseWiFiConfig(JsonObjectConst systemObj, WiFiConfig& outWiFi) {
  JsonObjectConst wifiObj = systemObj["wifi"].as<JsonObjectConst>();
  if (wifiObj.isNull()) {
    return true;
  }

  JsonObjectConst masterObj = wifiObj["master"].as<JsonObjectConst>();
  JsonObjectConst userObj = wifiObj["user"].as<JsonObjectConst>();
  if (!parseWiFiCredential(masterObj, outWiFi.master)) return false;
  if (!parseWiFiCredential(userObj, outWiFi.user)) return false;
  if (!wifiObj["retryBackoffSec"].is<uint32_t>() || !wifiObj["staOnly"].is<bool>()) {
    return false;
  }
  outWiFi.retryBackoffSec = wifiObj["retryBackoffSec"].as<uint32_t>();
  outWiFi.staOnly = wifiObj["staOnly"].as<bool>();
  return true;
}

bool parseFamily(const char* family, CardFamily& outFamily) {
  if (family == nullptr) return false;
  if (strcmp(family, "DI") == 0) {
    outFamily = CardFamily::DI;
    return true;
  }
  if (strcmp(family, "DO") == 0) {
    outFamily = CardFamily::DO;
    return true;
  }
  if (strcmp(family, "AI") == 0) {
    outFamily = CardFamily::AI;
    return true;
  }
  if (strcmp(family, "SIO") == 0) {
    outFamily = CardFamily::SIO;
    return true;
  }
  if (strcmp(family, "MATH") == 0) {
    outFamily = CardFamily::MATH;
    return true;
  }
  if (strcmp(family, "RTC") == 0) {
    outFamily = CardFamily::RTC;
    return true;
  }
  return false;
}

bool parseCommonCard(JsonObjectConst cardObj, CardConfig& outCard,
                     ConfigValidationError& outError, uint8_t cardIndex) {
  if (!cardObj["id"].is<uint8_t>() || !cardObj["enabled"].is<bool>() ||
      !cardObj["family"].is<const char*>()) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
    return false;
  }

  outCard.id = cardObj["id"].as<uint8_t>();
  outCard.enabled = cardObj["enabled"].as<bool>();

  CardFamily family = CardFamily::DI;
  if (!parseFamily(cardObj["family"].as<const char*>(), family)) {
    outError = {ConfigErrorCode::ConfigPayloadUnknownFamily, cardIndex};
    return false;
  }
  outCard.family = family;
  return true;
}

bool parseFamilyParams(JsonObjectConst cardObj, CardConfig& outCard,
                       ConfigValidationError& outError, uint8_t cardIndex) {
  JsonObjectConst params = cardObj["params"].as<JsonObjectConst>();
  if (params.isNull()) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
    return false;
  }

  switch (outCard.family) {
    case CardFamily::DI: {
      if (!params["invert"].is<bool>() || !params["debounceMs"].is<uint32_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.di.channel = params["channel"] | outCard.id;
      outCard.di.invert = params["invert"].as<bool>();
      outCard.di.debounceMs = params["debounceMs"].as<uint32_t>();
      outCard.di.edgeMode = params["edgeMode"] | 0U;  // 0:RISING,1:FALLING,2:CHANGE
      outCard.di.setEnabled = params["setEnabled"] | false;
      outCard.di.resetEnabled = params["resetEnabled"] | false;
      initDefaultConditionBlock(outCard.di.setCondition, outCard.id,
                                outCard.di.setEnabled);
      initDefaultConditionBlock(outCard.di.resetCondition, outCard.id,
                                outCard.di.resetEnabled);
      JsonObjectConst setObj = params["set"].as<JsonObjectConst>();
      JsonObjectConst resetObj = params["reset"].as<JsonObjectConst>();
      if (!setObj.isNull() &&
          !parseConditionBlockOptional(setObj, outCard.di.setCondition)) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      if (!resetObj.isNull() &&
          !parseConditionBlockOptional(resetObj, outCard.di.resetCondition)) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      return true;
    }
    case CardFamily::DO:
      if ((!params["channel"].isNull() && !params["channel"].is<uint8_t>()) ||
          (!params["invert"].isNull() && !params["invert"].is<bool>()) ||
          (!params["mode"].isNull() && !params["mode"].is<uint8_t>()) ||
          (!params["delayBeforeOnMs"].isNull() &&
           !params["delayBeforeOnMs"].is<uint32_t>()) ||
          (!params["activeDurationMs"].isNull() &&
           !params["activeDurationMs"].is<uint32_t>()) ||
          (!params["repeatCount"].isNull() &&
           !params["repeatCount"].is<uint32_t>())) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.dout.channel = params["channel"] | outCard.id;
      outCard.dout.invert = params["invert"] | false;
      outCard.dout.mode = params["mode"] | 5U;  // Mode_DO_Normal
      outCard.dout.delayBeforeOnMs = params["delayBeforeOnMs"] | 0U;
      outCard.dout.activeDurationMs = params["activeDurationMs"] | 0U;
      outCard.dout.repeatCount = params["repeatCount"] | 1U;
      initDefaultConditionBlock(outCard.dout.setCondition, outCard.id, false);
      initDefaultConditionBlock(outCard.dout.resetCondition, outCard.id, false);
      {
        JsonObjectConst setObj = params["set"].as<JsonObjectConst>();
        JsonObjectConst resetObj = params["reset"].as<JsonObjectConst>();
        if (!setObj.isNull() &&
            !parseConditionBlockOptional(setObj, outCard.dout.setCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
        if (!resetObj.isNull() &&
            !parseConditionBlockOptional(resetObj, outCard.dout.resetCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
      }
      return true;
    case CardFamily::AI:
      if ((!params["inputMin"].isNull() && !params["inputMin"].is<uint32_t>()) ||
          (!params["inputMax"].isNull() && !params["inputMax"].is<uint32_t>()) ||
          (!params["outputMin"].isNull() &&
           !params["outputMin"].is<uint32_t>()) ||
          (!params["outputMax"].isNull() &&
           !params["outputMax"].is<uint32_t>()) ||
          (!params["emaAlphaX100"].isNull() &&
           !params["emaAlphaX100"].is<uint32_t>())) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.ai.channel = params["channel"] | outCard.id;
      outCard.ai.inputMin = params["inputMin"] | 4U;
      outCard.ai.inputMax = params["inputMax"] | 20U;
      outCard.ai.outputMin = params["outputMin"] | 0U;
      outCard.ai.outputMax = params["outputMax"] | 100U;
      outCard.ai.emaAlphaX100 = params["emaAlphaX100"] | 100U;
      return true;
    case CardFamily::SIO:
      if ((!params["invert"].isNull() && !params["invert"].is<bool>()) ||
          (!params["mode"].isNull() && !params["mode"].is<uint8_t>()) ||
          (!params["delayBeforeOnMs"].isNull() &&
           !params["delayBeforeOnMs"].is<uint32_t>()) ||
          (!params["activeDurationMs"].isNull() &&
           !params["activeDurationMs"].is<uint32_t>()) ||
          (!params["repeatCount"].isNull() &&
           !params["repeatCount"].is<uint32_t>())) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.sio.invert = params["invert"] | false;
      outCard.sio.mode = params["mode"] | 5U;  // Mode_DO_Normal
      outCard.sio.delayBeforeOnMs = params["delayBeforeOnMs"] | 0U;
      outCard.sio.activeDurationMs = params["activeDurationMs"] | 0U;
      outCard.sio.repeatCount = params["repeatCount"] | 1U;
      initDefaultConditionBlock(outCard.sio.setCondition, outCard.id, false);
      initDefaultConditionBlock(outCard.sio.resetCondition, outCard.id, false);
      {
        JsonObjectConst setObj = params["set"].as<JsonObjectConst>();
        JsonObjectConst resetObj = params["reset"].as<JsonObjectConst>();
        if (!setObj.isNull() &&
            !parseConditionBlockOptional(setObj, outCard.sio.setCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
        if (!resetObj.isNull() &&
            !parseConditionBlockOptional(resetObj, outCard.sio.resetCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
      }
      return true;
    case CardFamily::MATH:
      if ((!params["operation"].isNull() && !params["operation"].is<uint8_t>()) ||
          (!params["inputA"].isNull() && !params["inputA"].is<uint32_t>()) ||
          (!params["inputB"].isNull() && !params["inputB"].is<uint32_t>()) ||
          (!params["inputMin"].isNull() && !params["inputMin"].is<uint32_t>()) ||
          (!params["inputMax"].isNull() && !params["inputMax"].is<uint32_t>()) ||
          (!params["outputMin"].isNull() &&
           !params["outputMin"].is<uint32_t>()) ||
          (!params["outputMax"].isNull() &&
           !params["outputMax"].is<uint32_t>()) ||
          (!params["emaAlphaX100"].isNull() &&
           !params["emaAlphaX100"].is<uint32_t>()) ||
          (!params["fallbackValue"].isNull() &&
           !params["fallbackValue"].is<uint32_t>())) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.math.operation = params["operation"] | 0U;
      outCard.math.inputA = params["inputA"] | 0U;
      outCard.math.inputB = params["inputB"] | 0U;
      outCard.math.inputMin = params["inputMin"] | 0U;
      outCard.math.inputMax = params["inputMax"] | 10000U;
      outCard.math.outputMin = params["outputMin"] | 0U;
      outCard.math.outputMax = params["outputMax"] | 10000U;
      outCard.math.emaAlphaX100 = params["emaAlphaX100"] | 100U;
      outCard.math.fallbackValue = params["fallbackValue"] | 0U;
      initDefaultConditionBlock(outCard.math.setCondition, outCard.id, false);
      initDefaultConditionBlock(outCard.math.resetCondition, outCard.id, false);
      {
        JsonObjectConst setObj = params["set"].as<JsonObjectConst>();
        JsonObjectConst resetObj = params["reset"].as<JsonObjectConst>();
        if (!setObj.isNull() &&
            !parseConditionBlockOptional(setObj, outCard.math.setCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
        if (!resetObj.isNull() &&
            !parseConditionBlockOptional(resetObj, outCard.math.resetCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
      }
      return true;
    case CardFamily::RTC:
      if (!params["hour"].is<uint8_t>() || !params["minute"].is<uint8_t>() ||
          !params["durationSeconds"].is<uint16_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.rtc.hour = params["hour"].as<uint8_t>();
      outCard.rtc.minute = params["minute"].as<uint8_t>();
      outCard.rtc.durationSeconds = params["durationSeconds"].as<uint16_t>();
      return true;
    default:
      outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
      return false;
  }
}

}  // namespace

ConfigDecodeResult decodeSystemConfig(JsonObjectConst root) {
  ConfigDecodeResult result = {};
  result.ok = false;
  result.error = {ConfigErrorCode::None, 0};
  result.decoded = makeDefaultSystemConfig();

  JsonObjectConst systemObj = root["system"].as<JsonObjectConst>();
  if (systemObj.isNull()) systemObj = root;

  if (!systemObj["schemaVersion"].is<uint32_t>() ||
      !systemObj["scanIntervalMs"].is<uint32_t>() ||
      !systemObj["cards"].is<JsonArrayConst>()) {
    result.error = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return result;
  }

  result.decoded.schemaVersion = systemObj["schemaVersion"].as<uint32_t>();
  result.decoded.scanIntervalMs = systemObj["scanIntervalMs"].as<uint32_t>();
  if (!parseWiFiConfig(systemObj, result.decoded.wifi)) {
    result.error = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return result;
  }

  JsonArrayConst cards = systemObj["cards"].as<JsonArrayConst>();
  if (cards.size() > kMaxCards) {
    result.error = {ConfigErrorCode::CardCountOutOfRange, 0};
    return result;
  }

  result.decoded.cardCount = static_cast<uint8_t>(cards.size());

  uint8_t cardIndex = 0;
  for (JsonObjectConst cardObj : cards) {
    CardConfig card = {};
    if (!parseCommonCard(cardObj, card, result.error, cardIndex)) return result;
    if (!parseFamilyParams(cardObj, card, result.error, cardIndex)) return result;
    result.decoded.cards[cardIndex] = card;
    ++cardIndex;
  }

  result.ok = true;
  return result;
}

}  // namespace v3::storage
