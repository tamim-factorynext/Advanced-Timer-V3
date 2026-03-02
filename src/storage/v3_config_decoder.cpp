#include "storage/v3_config_decoder.h"

#include <string.h>

namespace v3::storage {

namespace {

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
    case CardFamily::DI:
      if (!params["invert"].is<bool>() || !params["debounceMs"].is<uint32_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.di.invert = params["invert"].as<bool>();
      outCard.di.debounceMs = params["debounceMs"].as<uint32_t>();
      return true;
    case CardFamily::DO:
      if (!params["onDelayMs"].is<uint32_t>() ||
          !params["onDurationMs"].is<uint32_t>() ||
          !params["repeatCount"].is<uint32_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.dout.onDelayMs = params["onDelayMs"].as<uint32_t>();
      outCard.dout.onDurationMs = params["onDurationMs"].as<uint32_t>();
      outCard.dout.repeatCount = params["repeatCount"].as<uint32_t>();
      return true;
    case CardFamily::AI:
      if (!params["inputMin"].is<uint32_t>() || !params["inputMax"].is<uint32_t>() ||
          !params["outputMin"].is<uint32_t>() ||
          !params["outputMax"].is<uint32_t>() ||
          !params["emaAlphaX100"].is<uint32_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.ai.inputMin = params["inputMin"].as<uint32_t>();
      outCard.ai.inputMax = params["inputMax"].as<uint32_t>();
      outCard.ai.outputMin = params["outputMin"].as<uint32_t>();
      outCard.ai.outputMax = params["outputMax"].as<uint32_t>();
      outCard.ai.emaAlphaX100 = params["emaAlphaX100"].as<uint32_t>();
      return true;
    case CardFamily::SIO:
      if (!params["onDelayMs"].is<uint32_t>() ||
          !params["onDurationMs"].is<uint32_t>() ||
          !params["repeatCount"].is<uint32_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.sio.onDelayMs = params["onDelayMs"].as<uint32_t>();
      outCard.sio.onDurationMs = params["onDurationMs"].as<uint32_t>();
      outCard.sio.repeatCount = params["repeatCount"].as<uint32_t>();
      return true;
    case CardFamily::MATH:
      if (!params["thresholdCentiunits"].is<uint32_t>() ||
          !params["clampMin"].is<uint32_t>() ||
          !params["clampMax"].is<uint32_t>()) {
        outError = {ConfigErrorCode::ConfigPayloadInvalidShape, cardIndex};
        return false;
      }
      outCard.math.thresholdCentiunits =
          params["thresholdCentiunits"].as<uint32_t>();
      outCard.math.clampMin = params["clampMin"].as<uint32_t>();
      outCard.math.clampMax = params["clampMax"].as<uint32_t>();
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
