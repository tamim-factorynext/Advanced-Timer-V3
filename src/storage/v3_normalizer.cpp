#include "storage/v3_normalizer.h"

#include <cstring>
#include <string>

#include "kernel/v3_condition_rules.h"
#include "kernel/v3_card_types.h"
#include "kernel/v3_payload_rules.h"
#include "kernel/v3_typed_card_parser.h"

namespace {
logicCardType expectedTypeForCardId(const V3CardLayout& layout, uint8_t id) {
  if (id < layout.doStart) return DigitalInput;
  if (id < layout.aiStart) return DigitalOutput;
  if (id < layout.sioStart) return AnalogInput;
  if (id < layout.mathStart) return SoftIO;
  if (id < layout.rtcStart) return MathCard;
  return RtcCard;
}

bool hasLegacyCardsShape(JsonArrayConst cards) {
  for (JsonVariantConst v : cards) {
    if (!v.is<JsonObjectConst>()) continue;
    JsonObjectConst o = v.as<JsonObjectConst>();
    if (o["id"].is<uint64_t>() || o["type"].is<const char*>()) return true;
    break;
  }
  return false;
}

bool hasV3CardsShape(JsonArrayConst cards) {
  for (JsonVariantConst v : cards) {
    if (!v.is<JsonObjectConst>()) continue;
    JsonObjectConst o = v.as<JsonObjectConst>();
    if (o["cardId"].is<uint64_t>() || o["cardType"].is<const char*>()) return true;
    break;
  }
  return false;
}

}  // namespace

bool normalizeConfigRequestWithLayout(
    JsonObjectConst root, const V3CardLayout& layout, const char* apiVersion,
    const char* schemaVersion, const LogicCard* baselineCards,
    size_t baselineCount, JsonDocument& normalizedDoc, JsonArrayConst& outCards,
    String& reason, const char*& outErrorCode, V3RtcScheduleChannel* rtcOut,
    size_t rtcOutCount, V3CardConfig* typedOut, size_t typedOutCount) {
  outErrorCode = "VALIDATION_FAILED";
  const char* requestId = root["requestId"] | "";
  const char* reqApiVersion = root["apiVersion"].as<const char*>();
  const char* reqSchemaVersion = root["schemaVersion"].as<const char*>();

  if (reqApiVersion != nullptr && std::strcmp(reqApiVersion, apiVersion) != 0) {
    reason = "unsupported apiVersion";
    outErrorCode = "UNSUPPORTED_API_VERSION";
    return false;
  }
  if (reqSchemaVersion != nullptr &&
      std::strcmp(reqSchemaVersion, schemaVersion) != 0) {
    reason = "unsupported schemaVersion";
    outErrorCode = "UNSUPPORTED_SCHEMA_VERSION";
    return false;
  }
  if (!root["config"].is<JsonObjectConst>()) {
    reason = "missing config object";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }
  JsonObjectConst config = root["config"].as<JsonObjectConst>();
  if (!config["cards"].is<JsonArrayConst>()) {
    reason = "missing config.cards array";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }
  JsonArrayConst inputCards = config["cards"].as<JsonArrayConst>();

  normalizedDoc["requestId"] = requestId;
  normalizedDoc["apiVersion"] = apiVersion;
  normalizedDoc["schemaVersion"] = schemaVersion;

  for (size_t i = 0; i < rtcOutCount; ++i) {
    rtcOut[i].enabled = false;
    rtcOut[i].year = -1;
    rtcOut[i].month = -1;
    rtcOut[i].day = -1;
    rtcOut[i].weekday = -1;
    rtcOut[i].hour = -1;
    rtcOut[i].minute = -1;
    rtcOut[i].rtcCardId = static_cast<uint8_t>(layout.rtcStart + i);
  }

  if (hasLegacyCardsShape(inputCards)) {
    reason = "legacy cards payload is no longer supported; send V3 cardType/cardId cards";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }

  if (!hasV3CardsShape(inputCards)) {
    reason = "cards payload shape is unsupported";
    outErrorCode = "INVALID_REQUEST";
    return false;
  }

  {
    std::string payloadReason;
    if (!validateV3PayloadConditionSources(
            inputCards, layout.totalCards, layout.doStart, layout.aiStart,
            layout.sioStart, layout.mathStart, layout.rtcStart, payloadReason)) {
      reason = payloadReason.c_str();
      outErrorCode = "VALIDATION_FAILED";
      return false;
    }
  }

  if (baselineCount < layout.totalCards) {
    reason = "baseline card profile mismatch";
    outErrorCode = "INTERNAL_ERROR";
    return false;
  }

  V3CardConfig typedCards[255] = {};
  bool seen[255] = {};

  if (layout.totalCards > 255) {
    reason = "layout totalCards exceeds capacity";
    outErrorCode = "INTERNAL_ERROR";
    return false;
  }
  if (typedOut == nullptr || typedOutCount < layout.totalCards) {
    reason = "typed card output buffer mismatch";
    outErrorCode = "INTERNAL_ERROR";
    return false;
  }

  logicCardType sourceTypeById[255] = {};
  for (uint8_t i = 0; i < layout.totalCards; ++i) {
    (void)baselineCards;
    sourceTypeById[i] = expectedTypeForCardId(layout, i);
  }

  for (JsonVariantConst v : inputCards) {
    if (!v.is<JsonObjectConst>()) {
      reason = "cards[] item is not object";
      return false;
    }
    JsonObjectConst card = v.as<JsonObjectConst>();
    uint8_t cardId = card["cardId"] | 255;
    if (cardId >= layout.totalCards) {
      reason = "cardId out of range";
      return false;
    }
    if (seen[cardId]) {
      reason = "duplicate cardId";
      return false;
    }
    seen[cardId] = true;
    logicCardType parsedType = DigitalInput;
    if (!parseV3CardTypeToken(card["cardType"] | "", parsedType)) {
      reason = "invalid cardType";
      return false;
    }
    logicCardType expectedType = expectedTypeForCardId(layout, cardId);
    if (parsedType != expectedType) {
      reason = "cardType does not match cardId family slot";
      return false;
    }
    sourceTypeById[cardId] = parsedType;
  }

  for (JsonVariantConst v : inputCards) {
    JsonObjectConst card = v.as<JsonObjectConst>();
    uint8_t cardId = card["cardId"] | 255;
    if (!parseV3CardToTyped(card, sourceTypeById, layout.totalCards,
                            layout.doStart, layout.aiStart, layout.sioStart,
                            layout.mathStart, layout.rtcStart,
                            typedCards[cardId], reason)) {
      outErrorCode = "VALIDATION_FAILED";
      return false;
    }
  }

  for (uint8_t id = 0; id < layout.totalCards; ++id) {
    if (!seen[id]) {
      reason = "missing cardId in v3 payload";
      outErrorCode = "VALIDATION_FAILED";
      return false;
    }

    if (typedCards[id].family == V3CardFamily::RTC) {
      int slot = static_cast<int>(id) - static_cast<int>(layout.rtcStart);
      if (slot >= 0 && static_cast<size_t>(slot) < rtcOutCount) {
        rtcOut[slot].enabled = true;
        rtcOut[slot].year = typedCards[id].rtc.hasYear
                                ? static_cast<int16_t>(typedCards[id].rtc.year)
                                : static_cast<int16_t>(-1);
        rtcOut[slot].month = typedCards[id].rtc.hasMonth
                                 ? static_cast<int8_t>(typedCards[id].rtc.month)
                                 : static_cast<int8_t>(-1);
        rtcOut[slot].day = typedCards[id].rtc.hasDay
                               ? static_cast<int8_t>(typedCards[id].rtc.day)
                               : static_cast<int8_t>(-1);
        rtcOut[slot].weekday =
            typedCards[id].rtc.hasWeekday
                ? static_cast<int8_t>(typedCards[id].rtc.weekday)
                : static_cast<int8_t>(-1);
        rtcOut[slot].hour = static_cast<int8_t>(typedCards[id].rtc.hour);
        rtcOut[slot].minute = static_cast<int8_t>(typedCards[id].rtc.minute);
        rtcOut[slot].rtcCardId = id;
      }
    }
  }

  JsonObject outConfig = normalizedDoc["config"].to<JsonObject>();
  JsonArray out = outConfig["cards"].to<JsonArray>();
  for (JsonVariantConst v : inputCards) {
    JsonObject node = out.add<JsonObject>();
    if (!node.set(v.as<JsonObjectConst>())) {
      reason = "failed to normalize card payload";
      outErrorCode = "INTERNAL_ERROR";
      return false;
    }
  }

  for (uint8_t i = 0; i < layout.totalCards; ++i) {
    typedOut[i] = typedCards[i];
  }
  outCards = out;
  return true;
}
