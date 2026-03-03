/*
File: src/kernel/v3_condition_rules.cpp
Purpose: Implements the v3 condition rules module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/v3_payload_rules.cpp
- src/kernel/v3_typed_card_parser.cpp
- src/storage/v3_normalizer.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_condition_rules.h"

#include <cstring>

/**
 * @brief Parses V3 family token to legacy `logicCardType`.
 * @details Shared helper used by payload and typed-card parser validation paths.
 * @par Used By
 * - src/kernel/v3_payload_rules.cpp
 * - src/kernel/v3_typed_card_parser.cpp
 */
bool parseV3CardTypeToken(const char* cardType, logicCardType& outType) {
  if (cardType == nullptr) return false;
  if (std::strcmp(cardType, "DI") == 0) return (outType = DigitalInput), true;
  if (std::strcmp(cardType, "DO") == 0) return (outType = DigitalOutput), true;
  if (std::strcmp(cardType, "AI") == 0) return (outType = AnalogInput), true;
  if (std::strcmp(cardType, "SIO") == 0) return (outType = SoftIO), true;
  if (std::strcmp(cardType, "MATH") == 0) return (outType = MathCard), true;
  if (std::strcmp(cardType, "RTC") == 0) return (outType = RtcCard), true;
  return false;
}

/**
 * @brief Validates condition field token against source family capabilities.
 * @details Prevents invalid field usage before operator mapping/conversion.
 * @par Used By
 * - src/kernel/v3_payload_rules.cpp
 * - src/kernel/v3_typed_card_parser.cpp
 */
bool isV3FieldAllowedForSourceType(logicCardType sourceType, const char* field) {
  if (field == nullptr) return false;
  if (std::strcmp(field, "liveValue") == 0) return sourceType != RtcCard;
  if (sourceType == DigitalInput) {
    return std::strcmp(field, "commandState") == 0 ||
           std::strcmp(field, "actualState") == 0 ||
           std::strcmp(field, "edgePulse") == 0;
  }
  if (sourceType == DigitalOutput || sourceType == SoftIO) {
    return std::strcmp(field, "commandState") == 0 ||
           std::strcmp(field, "actualState") == 0 ||
           std::strcmp(field, "edgePulse") == 0 ||
           std::strcmp(field, "missionState") == 0;
  }
  if (sourceType == RtcCard) {
    return std::strcmp(field, "commandState") == 0 ||
           std::strcmp(field, "edgePulse") == 0;
  }
  if (sourceType == MathCard) {
    return std::strcmp(field, "edgePulse") == 0;
  }
  return false;
}

/**
 * @brief Validates operator token against selected condition field type.
 * @details Enforces boolean/mission/numeric operator compatibility.
 * @par Used By
 * - src/kernel/v3_payload_rules.cpp
 * - src/kernel/v3_typed_card_parser.cpp
 */
bool isV3OperatorAllowedForField(const char* field, const char* op) {
  if (field == nullptr || op == nullptr) return false;
  if (std::strcmp(field, "commandState") == 0 ||
      std::strcmp(field, "actualState") == 0 ||
      std::strcmp(field, "edgePulse") == 0) {
    return std::strcmp(op, "EQ") == 0 || std::strcmp(op, "NEQ") == 0;
  }
  if (std::strcmp(field, "missionState") == 0) {
    return std::strcmp(op, "EQ") == 0;
  }
  if (std::strcmp(field, "liveValue") == 0) {
    return std::strcmp(op, "GT") == 0 || std::strcmp(op, "GTE") == 0 ||
           std::strcmp(op, "LT") == 0 || std::strcmp(op, "LTE") == 0 ||
           std::strcmp(op, "EQ") == 0 || std::strcmp(op, "NEQ") == 0;
  }
  return false;
}

