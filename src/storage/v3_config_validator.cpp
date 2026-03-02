#include "storage/v3_config_validator.h"

namespace v3::storage {

namespace {

bool isNonEmpty(const char* text) {
  return text != nullptr && text[0] != '\0';
}

bool isDuplicateCardId(const SystemConfig& cfg, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = static_cast<uint8_t>(i + 1); j < count; ++j) {
      if (cfg.cards[i].id == cfg.cards[j].id) return true;
    }
  }
  return false;
}

bool isValidConditionOperator(v3::storage::ConditionOperator op) {
  return op >= v3::storage::ConditionOperator::AlwaysTrue &&
         op <= v3::storage::ConditionOperator::Stopped;
}

bool isValidConditionCombiner(v3::storage::ConditionCombiner combiner) {
  return combiner == v3::storage::ConditionCombiner::None ||
         combiner == v3::storage::ConditionCombiner::And ||
         combiner == v3::storage::ConditionCombiner::Or;
}

bool validateConditionBlock(const v3::storage::ConditionBlock& block, uint8_t cardCount) {
  if (!isValidConditionCombiner(block.combiner)) return false;
  if (block.clauseA.sourceCardId >= cardCount ||
      block.clauseB.sourceCardId >= cardCount) {
    return false;
  }
  if (!isValidConditionOperator(block.clauseA.op) ||
      !isValidConditionOperator(block.clauseB.op)) {
    return false;
  }
  return true;
}

}  // namespace

ConfigValidationResult validateSystemConfig(const SystemConfig& candidate) {
  ConfigValidationResult result = {};
  result.ok = false;
  result.error = {ConfigErrorCode::None, 0};

  if (candidate.schemaVersion != kConfigSchemaVersion) {
    result.error.code = ConfigErrorCode::SchemaVersionMismatch;
    return result;
  }

  if (candidate.scanIntervalMs < kMinScanIntervalMs ||
      candidate.scanIntervalMs > kMaxScanIntervalMs) {
    result.error.code = ConfigErrorCode::ScanIntervalOutOfRange;
    return result;
  }

  if (candidate.cardCount > kMaxCards) {
    result.error.code = ConfigErrorCode::CardCountOutOfRange;
    return result;
  }

  if (!candidate.wifi.staOnly || candidate.wifi.master.editable ||
      !isNonEmpty(candidate.wifi.master.ssid) ||
      !isNonEmpty(candidate.wifi.master.password) ||
      !isNonEmpty(candidate.wifi.user.ssid) ||
      !isNonEmpty(candidate.wifi.user.password) ||
      candidate.wifi.master.timeoutSec == 0 || candidate.wifi.user.timeoutSec == 0 ||
      candidate.wifi.retryBackoffSec == 0) {
    result.error.code = ConfigErrorCode::InvalidWiFiPolicy;
    return result;
  }

  if (isDuplicateCardId(candidate, candidate.cardCount)) {
    result.error.code = ConfigErrorCode::DuplicateCardId;
    return result;
  }

  for (uint8_t i = 0; i < candidate.cardCount; ++i) {
    const CardConfig& card = candidate.cards[i];

    if (card.family == CardFamily::DI && card.di.edgeMode > 2U) {
      result.error.code = ConfigErrorCode::InvalidDiMode;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::DO &&
        (card.dout.mode < 5U || card.dout.mode > 7U)) {
      result.error.code = ConfigErrorCode::InvalidDoMode;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::SIO &&
        (card.sio.mode < 5U || card.sio.mode > 7U)) {
      result.error.code = ConfigErrorCode::InvalidSioMode;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::DI &&
        (card.di.debounceMs % 10U) != 0U) {
      result.error.code = ConfigErrorCode::InvalidDiDebounceStep;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::DI &&
        (!validateConditionBlock(card.di.setCondition, candidate.cardCount) ||
         !validateConditionBlock(card.di.resetCondition, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::DO &&
        (!validateConditionBlock(card.dout.setCondition, candidate.cardCount) ||
         !validateConditionBlock(card.dout.resetCondition, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::SIO &&
        (!validateConditionBlock(card.sio.setCondition, candidate.cardCount) ||
         !validateConditionBlock(card.sio.resetCondition, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }

    if (card.ai.inputMin > card.ai.inputMax ||
        card.ai.outputMin > card.ai.outputMax ||
        card.ai.emaAlphaX100 > 100U) {
      result.error.code = ConfigErrorCode::InvalidAiRange;
      result.error.cardIndex = i;
      return result;
    }

    if (card.math.clampMin > card.math.clampMax) {
      result.error.code = ConfigErrorCode::InvalidMathClamp;
      result.error.cardIndex = i;
      return result;
    }

    if (card.rtc.hour > 23 || card.rtc.minute > 59) {
      result.error.code = ConfigErrorCode::InvalidRtcTime;
      result.error.cardIndex = i;
      return result;
    }
  }

  result.ok = true;
  result.validated.system = candidate;
  return result;
}

const char* configErrorCodeToString(ConfigErrorCode code) {
  switch (code) {
    case ConfigErrorCode::None:
      return "none";
    case ConfigErrorCode::SchemaVersionMismatch:
      return "schema_version_mismatch";
    case ConfigErrorCode::ScanIntervalOutOfRange:
      return "scan_interval_out_of_range";
    case ConfigErrorCode::CardCountOutOfRange:
      return "card_count_out_of_range";
    case ConfigErrorCode::DuplicateCardId:
      return "duplicate_card_id";
    case ConfigErrorCode::InvalidDiMode:
      return "invalid_di_mode";
    case ConfigErrorCode::InvalidDoMode:
      return "invalid_do_mode";
    case ConfigErrorCode::InvalidSioMode:
      return "invalid_sio_mode";
    case ConfigErrorCode::InvalidDiDebounceStep:
      return "invalid_di_debounce_step";
    case ConfigErrorCode::InvalidConditionBlock:
      return "invalid_condition_block";
    case ConfigErrorCode::InvalidAiRange:
      return "invalid_ai_range";
    case ConfigErrorCode::InvalidMathClamp:
      return "invalid_math_clamp";
    case ConfigErrorCode::InvalidRtcTime:
      return "invalid_rtc_time";
    case ConfigErrorCode::InvalidWiFiPolicy:
      return "invalid_wifi_policy";
    case ConfigErrorCode::ConfigPayloadInvalidJson:
      return "config_payload_invalid_json";
    case ConfigErrorCode::ConfigPayloadInvalidShape:
      return "config_payload_invalid_shape";
    case ConfigErrorCode::ConfigPayloadUnknownFamily:
      return "config_payload_unknown_family";
    default:
      return "unknown";
  }
}

}  // namespace v3::storage
