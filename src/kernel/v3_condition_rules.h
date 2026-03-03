/*
File: src/kernel/v3_condition_rules.h
Purpose: Declares the v3 condition rules module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/v3_payload_rules.cpp
- src/kernel/v3_typed_card_parser.cpp
- src/storage/v3_normalizer.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "kernel/card_model.h"

/**
 * @brief Parses V3 card type token to legacy `logicCardType`.
 * @details Used by payload normalization/parser validation stages.
 * @par Used By
 * - src/kernel/v3_payload_rules.cpp
 * - src/kernel/v3_typed_card_parser.cpp
 */
bool parseV3CardTypeToken(const char* cardType, logicCardType& outType);
/**
 * @brief Checks whether a source family supports a requested condition field.
 * @details Prevents field/operator combinations that have no runtime semantics.
 * @par Used By
 * - src/kernel/v3_payload_rules.cpp
 * - src/kernel/v3_typed_card_parser.cpp
 */
bool isV3FieldAllowedForSourceType(logicCardType sourceType, const char* field);
/**
 * @brief Checks whether operator token is valid for a selected condition field.
 * @details Enforces operator vocabulary compatibility per field kind.
 * @par Used By
 * - src/kernel/v3_payload_rules.cpp
 * - src/kernel/v3_typed_card_parser.cpp
 */
bool isV3OperatorAllowedForField(const char* field, const char* op);
