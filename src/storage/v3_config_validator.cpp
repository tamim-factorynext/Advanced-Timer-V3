#include "storage/v3_config_validator.h"

namespace v3::storage {

namespace {

bool isDuplicateCardId(const SystemConfig& cfg, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = static_cast<uint8_t>(i + 1); j < count; ++j) {
      if (cfg.cards[i].id == cfg.cards[j].id) return true;
    }
  }
  return false;
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

  if (isDuplicateCardId(candidate, candidate.cardCount)) {
    result.error.code = ConfigErrorCode::DuplicateCardId;
    return result;
  }

  for (uint8_t i = 0; i < candidate.cardCount; ++i) {
    const CardConfig& card = candidate.cards[i];

    if (card.ai.inputMin > card.ai.inputMax ||
        card.ai.outputMin > card.ai.outputMax) {
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
    case ConfigErrorCode::InvalidAiRange:
      return "invalid_ai_range";
    case ConfigErrorCode::InvalidMathClamp:
      return "invalid_math_clamp";
    case ConfigErrorCode::InvalidRtcTime:
      return "invalid_rtc_time";
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
