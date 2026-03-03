/*
File: src/storage/v3_config_validator.cpp
Purpose: Implements the v3 config validator module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/storage/storage_service.cpp
- src/storage/v3_config_decoder.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
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

bool isNumericConditionOperator(v3::storage::ConditionOperator op) {
  return op == v3::storage::ConditionOperator::GT ||
         op == v3::storage::ConditionOperator::LT ||
         op == v3::storage::ConditionOperator::EQ ||
         op == v3::storage::ConditionOperator::NEQ ||
         op == v3::storage::ConditionOperator::GTE ||
         op == v3::storage::ConditionOperator::LTE;
}

bool familyHasNumericLiveValue(v3::storage::CardFamily family) {
  return family != v3::storage::CardFamily::RTC;
}

bool validateConditionClause(const v3::storage::ConditionClause& clause,
                             const v3::storage::SystemConfig& cfg,
                             uint8_t cardCount) {
  if (clause.sourceCardId >= cardCount) return false;
  if (!isValidConditionOperator(clause.op)) return false;
  if (!clause.useThresholdCard) return true;

  if (!isNumericConditionOperator(clause.op)) return false;
  if (clause.thresholdCardId >= cardCount) return false;
  if (clause.sourceCardId == clause.thresholdCardId) return false;
  const v3::storage::CardConfig& thresholdCard = cfg.cards[clause.thresholdCardId];
  return familyHasNumericLiveValue(thresholdCard.family);
}

bool validateConditionBlock(const v3::storage::ConditionBlock& block,
                            const v3::storage::SystemConfig& cfg,
                            uint8_t cardCount) {
  if (!isValidConditionCombiner(block.combiner)) return false;
  if (!validateConditionClause(block.clauseA, cfg, cardCount)) return false;
  return validateConditionClause(block.clauseB, cfg, cardCount);
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

  if (candidate.scanPeriodMs < kMinScanIntervalMs ||
      candidate.scanPeriodMs > kMaxScanIntervalMs) {
    result.error.code = ConfigErrorCode::ScanIntervalOutOfRange;
    return result;
  }

  if (candidate.cardCount > kMaxCards) {
    result.error.code = ConfigErrorCode::CardCountOutOfRange;
    return result;
  }

  if (!candidate.wifi.staOnly || candidate.wifi.backupAccessNetwork.editable ||
      !isNonEmpty(candidate.wifi.backupAccessNetwork.ssid) ||
      !isNonEmpty(candidate.wifi.backupAccessNetwork.password) ||
      !isNonEmpty(candidate.wifi.userConfiguredNetwork.ssid) ||
      !isNonEmpty(candidate.wifi.userConfiguredNetwork.password) ||
      candidate.wifi.backupAccessNetwork.timeoutSec == 0 || candidate.wifi.userConfiguredNetwork.timeoutSec == 0 ||
      candidate.wifi.retryDelaySec == 0) {
    result.error.code = ConfigErrorCode::InvalidWiFiPolicy;
    return result;
  }

  if (!isNonEmpty(candidate.time.timezone) ||
      !isNonEmpty(candidate.time.timeSync.primaryTimeServer) ||
      !isNonEmpty(candidate.time.timeSync.secondaryServer) ||
      !isNonEmpty(candidate.time.timeSync.tertiaryServer) ||
      candidate.time.timeSync.syncIntervalSec == 0 ||
      candidate.time.timeSync.startupTimeoutSec == 0 ||
      candidate.time.timeSync.maxTimeAgeSec == 0) {
    result.error.code = ConfigErrorCode::ConfigPayloadInvalidShape;
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
        (!validateConditionBlock(card.di.turnOnCondition, candidate, candidate.cardCount) ||
         !validateConditionBlock(card.di.turnOffCondition, candidate, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::DO &&
        (!validateConditionBlock(card.dout.turnOnCondition, candidate, candidate.cardCount) ||
         !validateConditionBlock(card.dout.turnOffCondition, candidate, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::SIO &&
        (!validateConditionBlock(card.sio.turnOnCondition, candidate, candidate.cardCount) ||
         !validateConditionBlock(card.sio.turnOffCondition, candidate, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }
    if (card.family == CardFamily::MATH &&
        (!validateConditionBlock(card.math.turnOnCondition, candidate, candidate.cardCount) ||
         !validateConditionBlock(card.math.turnOffCondition, candidate, candidate.cardCount))) {
      result.error.code = ConfigErrorCode::InvalidConditionBlock;
      result.error.cardIndex = i;
      return result;
    }

    if (card.ai.inputMin > card.ai.inputMax ||
        card.ai.outputMin > card.ai.outputMax ||
        card.ai.smoothingFactorPct > 100U) {
      result.error.code = ConfigErrorCode::InvalidAiRange;
      result.error.cardIndex = i;
      return result;
    }

    if (card.math.operation > 3U || card.math.inputMin >= card.math.inputMax ||
        card.math.smoothingFactorPct > 100U) {
      result.error.code = ConfigErrorCode::InvalidMathClamp;
      result.error.cardIndex = i;
      return result;
    }

    if (card.rtc.hasMonth && (card.rtc.month < 1 || card.rtc.month > 12)) {
      result.error.code = ConfigErrorCode::InvalidRtcTime;
      result.error.cardIndex = i;
      return result;
    }
    if (card.rtc.hasDay && (card.rtc.day < 1 || card.rtc.day > 31)) {
      result.error.code = ConfigErrorCode::InvalidRtcTime;
      result.error.cardIndex = i;
      return result;
    }
    if (card.rtc.hasWeekday && card.rtc.weekday > 6) {
      result.error.code = ConfigErrorCode::InvalidRtcTime;
      result.error.cardIndex = i;
      return result;
    }
    if (card.rtc.hasHour && card.rtc.hour > 23) {
      result.error.code = ConfigErrorCode::InvalidRtcTime;
      result.error.cardIndex = i;
      return result;
    }
    if (card.rtc.minute > 59) {
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





