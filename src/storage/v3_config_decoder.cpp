/*
File: src/storage/v3_config_decoder.cpp
Purpose: Implements the v3 config decoder module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/storage/storage_service.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "storage/v3_config_decoder.h"

#include <string.h>

namespace v3::storage {

namespace {

/**
 * @brief Initializes a condition block to safe disabled defaults.
 * @param block Destination condition block.
 * @param selfCardId Card id used as default clause source.
 * @param enabled Reserved compatibility flag; currently unused.
 * @par Used By
 * parseFamilyParams().
 */
void initDefaultConditionBlock(v3::storage::ConditionBlock& block, uint8_t selfCardId,
                               bool enabled) {
  (void)enabled;
  block.combiner = v3::storage::ConditionCombiner::None;
  block.clauseA.sourceCardId = selfCardId;
  block.clauseA.op = v3::storage::ConditionOperator::AlwaysFalse;
  block.clauseA.thresholdValue = 0;
  block.clauseA.thresholdCardId = selfCardId;
  block.clauseA.useThresholdCard = false;
  block.clauseB.sourceCardId = selfCardId;
  block.clauseB.op = v3::storage::ConditionOperator::AlwaysFalse;
  block.clauseB.thresholdValue = 0;
  block.clauseB.thresholdCardId = selfCardId;
  block.clauseB.useThresholdCard = false;
}

/**
 * @brief Parses condition operator token into typed enum.
 * @details Accepts canonical operator tokens only.
 * @param token Operator token.
 * @param out Parsed operator enum.
 * @retval true Token recognized.
 * @retval false Token invalid.
 * @par Used By
 * parseConditionClause().
 */
bool parseConditionOperator(const char* token, v3::storage::ConditionOperator& out) {
  if (token == nullptr) return false;
  if (strcmp(token, "ALWAYS_TRUE") == 0) {
    out = v3::storage::ConditionOperator::AlwaysTrue;
    return true;
  }
  if (strcmp(token, "ALWAYS_FALSE") == 0) {
    out = v3::storage::ConditionOperator::AlwaysFalse;
    return true;
  }
  if (strcmp(token, "LOGICAL_TRUE") == 0) {
    out = v3::storage::ConditionOperator::LogicalTrue;
    return true;
  }
  if (strcmp(token, "LOGICAL_FALSE") == 0) {
    out = v3::storage::ConditionOperator::LogicalFalse;
    return true;
  }
  if (strcmp(token, "PHYSICAL_ON") == 0) {
    out = v3::storage::ConditionOperator::PhysicalOn;
    return true;
  }
  if (strcmp(token, "PHYSICAL_OFF") == 0) {
    out = v3::storage::ConditionOperator::PhysicalOff;
    return true;
  }
  if (strcmp(token, "TRIGGERED") == 0) {
    out = v3::storage::ConditionOperator::Triggered;
    return true;
  }
  if (strcmp(token, "TRIGGER_CLEARED") == 0) {
    out = v3::storage::ConditionOperator::TriggerCleared;
    return true;
  }
  if (strcmp(token, "GT") == 0) {
    out = v3::storage::ConditionOperator::GT;
    return true;
  }
  if (strcmp(token, "LT") == 0) {
    out = v3::storage::ConditionOperator::LT;
    return true;
  }
  if (strcmp(token, "EQ") == 0) {
    out = v3::storage::ConditionOperator::EQ;
    return true;
  }
  if (strcmp(token, "NEQ") == 0) {
    out = v3::storage::ConditionOperator::NEQ;
    return true;
  }
  if (strcmp(token, "GTE") == 0) {
    out = v3::storage::ConditionOperator::GTE;
    return true;
  }
  if (strcmp(token, "LTE") == 0) {
    out = v3::storage::ConditionOperator::LTE;
    return true;
  }
  if (strcmp(token, "RUNNING") == 0) {
    out = v3::storage::ConditionOperator::Running;
    return true;
  }
  if (strcmp(token, "FINISHED") == 0) {
    out = v3::storage::ConditionOperator::Finished;
    return true;
  }
  if (strcmp(token, "STOPPED") == 0) {
    out = v3::storage::ConditionOperator::Stopped;
    return true;
  }
  return false;
}

/**
 * @brief Parses condition combiner token.
 * @param token Combiner text (NONE/AND/OR).
 * @param out Parsed combiner.
 * @retval true Token recognized.
 * @retval false Token invalid.
 * @par Used By
 * parseConditionBlockOptional().
 */
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

/**
 * @brief Parses one condition clause object.
 * @param obj Clause JSON object.
 * @param outClause Parsed clause destination.
 * @retval true Clause is valid and parsed.
 * @retval false Clause shape or values are invalid.
 * @par Used By
 * parseConditionBlockOptional().
 */
bool parseConditionClause(JsonObjectConst obj, v3::storage::ConditionClause& outClause) {
  if (!obj["sourceCardId"].is<uint8_t>()) return false;
  if (!obj["thresholdValue"].is<uint32_t>()) return false;
  if (!obj["thresholdCardId"].is<uint8_t>()) return false;
  if (!obj["useThresholdCard"].is<bool>()) return false;
  const char* opToken = obj["operator"].as<const char*>();
  v3::storage::ConditionOperator op = v3::storage::ConditionOperator::AlwaysFalse;
  if (!parseConditionOperator(opToken, op)) return false;
  outClause.sourceCardId = obj["sourceCardId"].as<uint8_t>();
  outClause.op = op;
  outClause.thresholdValue = obj["thresholdValue"].as<uint32_t>();
  outClause.thresholdCardId = obj["thresholdCardId"].as<uint8_t>();
  outClause.useThresholdCard = obj["useThresholdCard"].as<bool>();
  return true;
}

/**
 * @brief Parses optional condition block with combiner and one/two clauses.
 * @param obj Condition block JSON object.
 * @param outBlock Parsed block destination.
 * @retval true Block parsed.
 * @retval false Block missing or invalid.
 * @par Used By
 * parseFamilyParams().
 */
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
    outBlock.clauseB.thresholdValue = 0;
    outBlock.clauseB.thresholdCardId = outBlock.clauseA.sourceCardId;
    outBlock.clauseB.useThresholdCard = false;
    return true;
  }

  if (!obj["clauseB"].is<JsonObjectConst>()) return false;
  if (!parseConditionClause(obj["clauseB"].as<JsonObjectConst>(), outBlock.clauseB)) {
    return false;
  }
  return true;
}

/**
 * @brief Parses one required bounded string field.
 * @param obj Source JSON object.
 * @param key Field key.
 * @param out Destination buffer.
 * @param outSize Destination buffer size.
 * @retval true Field present and copied.
 * @retval false Field missing/empty/too long.
 * @par Used By
 * parseWiFiCredential().
 */
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

/**
 * @brief Parses Wi-Fi credential object.
 * @param obj Credential JSON object.
 * @param outCredential Parsed credential destination.
 * @retval true Credential is valid.
 * @retval false Credential shape invalid.
 * @par Used By
 * parseWiFiConfig().
 */
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

/**
 * @brief Parses top-level Wi-Fi config section.
 * @details Expects `backupAccessNetwork`, `userConfiguredNetwork`,
 * `retryDelaySec`, and `staOnly` when `wifi` section exists.
 * @param systemObj Root/system JSON object.
 * @param outWiFi Parsed Wi-Fi config destination.
 * @retval true Wi-Fi section absent or valid.
 * @retval false Wi-Fi section present but invalid.
 * @par Used By
 * decodeSystemConfig().
 */
bool parseWiFiConfig(JsonObjectConst systemObj, WiFiConfig& outWiFi) {
  JsonObjectConst wifiObj = systemObj["wifi"].as<JsonObjectConst>();
  if (wifiObj.isNull()) {
    return true;
  }

  JsonObjectConst masterObj =
      wifiObj["backupAccessNetwork"].as<JsonObjectConst>();
  JsonObjectConst userObj =
      wifiObj["userConfiguredNetwork"].as<JsonObjectConst>();
  if (!parseWiFiCredential(masterObj, outWiFi.backupAccessNetwork)) return false;
  if (!parseWiFiCredential(userObj, outWiFi.userConfiguredNetwork)) return false;
  if (!wifiObj["retryDelaySec"].is<uint32_t>() || !wifiObj["staOnly"].is<bool>()) {
    return false;
  }
  outWiFi.retryDelaySec = wifiObj["retryDelaySec"].as<uint32_t>();
  outWiFi.staOnly = wifiObj["staOnly"].as<bool>();
  return true;
}

/**
 * @brief Parses optional bounded string field.
 * @param obj Source JSON object.
 * @param key Field key.
 * @param out Destination buffer.
 * @param outSize Destination buffer size.
 * @retval true Field present and copied.
 * @retval false Field missing/null/too long.
 * @par Used By
 * parseClockConfig().
 */
bool parseOptionalStringField(JsonObjectConst obj, const char* key, char* out,
                              size_t outSize) {
  if (!obj[key].is<const char*>()) return false;
  const char* value = obj[key].as<const char*>();
  if (value == nullptr) return false;
  const size_t len = strlen(value);
  if (len >= outSize) return false;
  memcpy(out, value, len);
  out[len] = '\0';
  return true;
}

/**
 * @brief Parses top-level time-sync config section.
 * @param systemObj Root/system JSON object.
 * @param outClock Parsed time config destination.
 * @retval true Time section absent or valid.
 * @retval false Time section present but invalid.
 * @par Used By
 * decodeSystemConfig().
 */
bool parseClockConfig(JsonObjectConst systemObj, ClockConfig& outClock) {
  JsonObjectConst clockObj = systemObj["time"].as<JsonObjectConst>();
  if (clockObj.isNull()) {
    return true;
  }

  if (!parseOptionalStringField(clockObj, "timezone", outClock.timezone,
                                sizeof(outClock.timezone))) {
    return false;
  }

  JsonObjectConst ntpObj = clockObj["timeSync"].as<JsonObjectConst>();
  if (ntpObj.isNull()) return false;
  if (!ntpObj["enabled"].is<bool>() || !ntpObj["syncIntervalSec"].is<uint32_t>() ||
      !ntpObj["startupTimeoutSec"].is<uint32_t>() ||
      !ntpObj["maxTimeAgeSec"].is<uint32_t>()) {
    return false;
  }

  if (!parseOptionalStringField(ntpObj, "primaryTimeServer", outClock.timeSync.primaryTimeServer,
                                sizeof(outClock.timeSync.primaryTimeServer)) ||
      !parseOptionalStringField(ntpObj, "secondaryServer",
                                outClock.timeSync.secondaryServer,
                                sizeof(outClock.timeSync.secondaryServer)) ||
      !parseOptionalStringField(ntpObj, "tertiaryServer",
                                outClock.timeSync.tertiaryServer,
                                sizeof(outClock.timeSync.tertiaryServer))) {
    return false;
  }

  outClock.timeSync.enabled = ntpObj["enabled"].as<bool>();
  outClock.timeSync.syncIntervalSec = ntpObj["syncIntervalSec"].as<uint32_t>();
  outClock.timeSync.startupTimeoutSec = ntpObj["startupTimeoutSec"].as<uint32_t>();
  outClock.timeSync.maxTimeAgeSec = ntpObj["maxTimeAgeSec"].as<uint32_t>();
  return true;
}

/**
 * @brief Parses card family token.
 * @param family Family token string.
 * @param outFamily Parsed family enum.
 * @retval true Token recognized.
 * @retval false Token invalid.
 * @par Used By
 * parseCommonCard().
 */
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

/**
 * @brief Parses family-agnostic common card fields.
 * @param cardObj Card JSON object.
 * @param outCard Card destination.
 * @param outError Decode error sink.
 * @param cardIndex Index used in error context.
 * @retval true Common fields parsed.
 * @retval false Invalid shape/family; error populated.
 * @par Used By
 * decodeSystemConfig().
 */
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

/**
 * @brief Parses family-specific `params` payload for one card.
 * @details Applies defaults for optional fields and validates required ones
 * per family contract.
 * @param cardObj Card JSON object.
 * @param outCard Card destination.
 * @param outError Decode error sink.
 * @param cardIndex Index used in error context.
 * @retval true Params parsed for card family.
 * @retval false Params invalid; error populated.
 * @par Used By
 * decodeSystemConfig().
 */
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
      initDefaultConditionBlock(outCard.di.turnOnCondition, outCard.id,
                                outCard.di.setEnabled);
      initDefaultConditionBlock(outCard.di.turnOffCondition, outCard.id,
                                outCard.di.resetEnabled);
      JsonObjectConst setObj = params["turnOnCondition"].as<JsonObjectConst>();
      JsonObjectConst resetObj = params["turnOffCondition"].as<JsonObjectConst>();
      if (!setObj.isNull() &&
          !parseConditionBlockOptional(setObj, outCard.di.turnOnCondition)) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      if (!resetObj.isNull() &&
          !parseConditionBlockOptional(resetObj, outCard.di.turnOffCondition)) {
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
      initDefaultConditionBlock(outCard.dout.turnOnCondition, outCard.id, false);
      initDefaultConditionBlock(outCard.dout.turnOffCondition, outCard.id, false);
      {
        JsonObjectConst setObj = params["turnOnCondition"].as<JsonObjectConst>();
        JsonObjectConst resetObj = params["turnOffCondition"].as<JsonObjectConst>();
        if (!setObj.isNull() &&
            !parseConditionBlockOptional(setObj, outCard.dout.turnOnCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
        if (!resetObj.isNull() &&
            !parseConditionBlockOptional(resetObj, outCard.dout.turnOffCondition)) {
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
          (!params["smoothingFactorPct"].isNull() &&
           !params["smoothingFactorPct"].is<uint32_t>())) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.ai.channel = params["channel"] | outCard.id;
      outCard.ai.inputMin = params["inputMin"] | 4U;
      outCard.ai.inputMax = params["inputMax"] | 20U;
      outCard.ai.outputMin = params["outputMin"] | 0U;
      outCard.ai.outputMax = params["outputMax"] | 100U;
      outCard.ai.smoothingFactorPct = params["smoothingFactorPct"] | 100U;
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
      initDefaultConditionBlock(outCard.sio.turnOnCondition, outCard.id, false);
      initDefaultConditionBlock(outCard.sio.turnOffCondition, outCard.id, false);
      {
        JsonObjectConst setObj = params["turnOnCondition"].as<JsonObjectConst>();
        JsonObjectConst resetObj = params["turnOffCondition"].as<JsonObjectConst>();
        if (!setObj.isNull() &&
            !parseConditionBlockOptional(setObj, outCard.sio.turnOnCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
        if (!resetObj.isNull() &&
            !parseConditionBlockOptional(resetObj, outCard.sio.turnOffCondition)) {
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
          (!params["smoothingFactorPct"].isNull() &&
           !params["smoothingFactorPct"].is<uint32_t>()) ||
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
      outCard.math.smoothingFactorPct = params["smoothingFactorPct"] | 100U;
      outCard.math.fallbackValue = params["fallbackValue"] | 0U;
      initDefaultConditionBlock(outCard.math.turnOnCondition, outCard.id, false);
      initDefaultConditionBlock(outCard.math.turnOffCondition, outCard.id, false);
      {
        JsonObjectConst setObj = params["turnOnCondition"].as<JsonObjectConst>();
        JsonObjectConst resetObj = params["turnOffCondition"].as<JsonObjectConst>();
        if (!setObj.isNull() &&
            !parseConditionBlockOptional(setObj, outCard.math.turnOnCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
        if (!resetObj.isNull() &&
            !parseConditionBlockOptional(resetObj, outCard.math.turnOffCondition)) {
          outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
          return false;
        }
      }
      return true;
    case CardFamily::RTC:
      if (!params["minute"].is<uint8_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.rtc.hasYear = params["hasYear"] | false;
      outCard.rtc.hasMonth = params["hasMonth"] | false;
      outCard.rtc.hasDay = params["hasDay"] | false;
      outCard.rtc.hasWeekday = params["hasWeekday"] | false;
      outCard.rtc.hasHour = params["hasHour"] | false;

      if (outCard.rtc.hasYear && !params["year"].is<uint16_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      if (outCard.rtc.hasMonth && !params["month"].is<uint8_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      if (outCard.rtc.hasDay && !params["day"].is<uint8_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      if (outCard.rtc.hasWeekday && !params["weekday"].is<uint8_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      if (outCard.rtc.hasHour && !params["hour"].is<uint8_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }

      outCard.rtc.year = params["year"] | 0U;
      outCard.rtc.month = params["month"] | 0U;
      outCard.rtc.day = params["day"] | 0U;
      outCard.rtc.weekday = params["weekday"] | 0U;
      outCard.rtc.hour = params["hour"] | 0U;
      outCard.rtc.minute = params["minute"].as<uint8_t>();
      outCard.rtc.triggerDurationMs = params["triggerDurationMs"] | 0U;
      return true;
    default:
      outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
      return false;
  }
}

}  // namespace

/**
 * @brief Decodes external config JSON into strongly typed SystemConfig.
 * @details Accepts both wrapped (`root.system`) and direct system shapes,
 * then parses core system fields and every card entry.
 * @param root Root JSON object from payload/file.
 * @return Decode result with `ok`, parsed payload, and error context.
 * @par Used By
 * StorageService::begin().
 */
bool decodeSystemConfigLight(JsonObjectConst root, SystemConfig& outDecoded,
                             ConfigValidationError& outError) {
  outError = {ConfigErrorCode::None, 0};
  makeDefaultSystemConfig(outDecoded);

  if (!decodeSystemSettingsLight(root, outDecoded, outError)) return false;

  JsonObjectConst systemObj = root["system"].as<JsonObjectConst>();
  if (systemObj.isNull()) systemObj = root;
  if (!systemObj["cards"].is<JsonArrayConst>()) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return false;
  }

  JsonArrayConst cards = systemObj["cards"].as<JsonArrayConst>();
  if (cards.size() > kMaxCards) {
    outError = {ConfigErrorCode::CardCountOutOfRange, 0};
    return false;
  }

  outDecoded.cardCount = static_cast<uint8_t>(cards.size());

  uint8_t cardIndex = 0;
  for (JsonObjectConst cardObj : cards) {
    CardConfig card = {};
    if (!decodeCardConfigLight(cardObj, card, outError, cardIndex)) return false;
    outDecoded.cards[cardIndex] = card;
    ++cardIndex;
  }

  return true;
}

bool decodeSystemSettingsLight(JsonObjectConst root, SystemConfig& inOutConfig,
                               ConfigValidationError& outError) {
  outError = {ConfigErrorCode::None, 0};
  JsonObjectConst systemObj = root["system"].as<JsonObjectConst>();
  if (systemObj.isNull()) systemObj = root;

  if (!systemObj["schemaVersion"].is<uint32_t>() ||
      !systemObj["scanPeriodMs"].is<uint32_t>()) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return false;
  }
  inOutConfig.schemaVersion = systemObj["schemaVersion"].as<uint32_t>();
  inOutConfig.scanPeriodMs = systemObj["scanPeriodMs"].as<uint32_t>();
  if (!parseWiFiConfig(systemObj, inOutConfig.wifi)) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return false;
  }
  if (!parseClockConfig(systemObj, inOutConfig.time)) {
    outError = {ConfigErrorCode::ConfigPayloadInvalidShape, 0};
    return false;
  }
  return true;
}

bool decodeCardConfigLight(JsonObjectConst cardObj, CardConfig& outCard,
                           ConfigValidationError& outError, uint8_t cardIndex) {
  outCard = {};
  if (!parseCommonCard(cardObj, outCard, outError, cardIndex)) return false;
  if (!parseFamilyParams(cardObj, outCard, outError, cardIndex)) return false;
  return true;
}

ConfigDecodeResult decodeSystemConfig(JsonObjectConst root) {
  ConfigDecodeResult result = {};
  result.ok = decodeSystemConfigLight(root, result.decoded, result.error);
  return result;
}

/*
 * Compatibility wrapper kept for call sites that still prefer envelope-style
 * decode flow.
 */

}  // namespace v3::storage





