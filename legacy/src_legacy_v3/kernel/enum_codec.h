/*
File: src/kernel/enum_codec.h
Purpose: Declares the enum codec module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/legacy_card_profile.cpp
- src/kernel/legacy_config_validator.cpp
- src/portal/portal_service.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include "control/command_dto.h"
#include "kernel/card_model.h"

/**
 * @brief Converts enum value to stable token string.
 * @details Used for diagnostics, JSON projection, and legacy/profile mapping paths.
 * @par Used By
 * - src/kernel/legacy_card_profile.cpp
 * - src/kernel/legacy_config_validator.cpp
 * - src/portal/portal_service.cpp
 */
const char* toString(logicCardType value);
/** @copydoc toString(logicCardType) */
const char* toString(logicOperator value);
/** @copydoc toString(logicCardType) */
const char* toString(cardMode value);
/** @copydoc toString(logicCardType) */
const char* toString(cardState value);
/** @copydoc toString(logicCardType) */
const char* toString(combineMode value);
/** @copydoc toString(logicCardType) */
const char* toString(engineMode value);
/** @copydoc toString(logicCardType) */
const char* toString(inputSourceMode value);

/**
 * @brief Attempts to parse token string into enum value.
 * @details Returns false on unknown token and leaves deterministic caller fallback in control.
 * @par Used By
 * - src/kernel/legacy_config_validator.cpp
 * - src/kernel/legacy_v3_typed_card_parser.cpp
 */
bool tryParseLogicCardType(const char* s, logicCardType& out);
/** @copydoc tryParseLogicCardType */
bool tryParseLogicOperator(const char* s, logicOperator& out);
/** @copydoc tryParseLogicCardType */
bool tryParseCardMode(const char* s, cardMode& out);
/** @copydoc tryParseLogicCardType */
bool tryParseCardState(const char* s, cardState& out);
/** @copydoc tryParseLogicCardType */
bool tryParseCombineMode(const char* s, combineMode& out);

/**
 * @brief Parses token into enum with explicit fallback.
 * @details Ensures caller can continue with deterministic default when parse fails.
 * @par Used By
 * - src/kernel/legacy_card_profile.cpp
 * - src/kernel/legacy_config_validator.cpp
 */
logicCardType parseOrDefault(const char* s, logicCardType fallback);
/** @copydoc parseOrDefault(const char*, logicCardType) */
logicOperator parseOrDefault(const char* s, logicOperator fallback);
/** @copydoc parseOrDefault(const char*, logicCardType) */
cardMode parseOrDefault(const char* s, cardMode fallback);
/** @copydoc parseOrDefault(const char*, logicCardType) */
cardState parseOrDefault(const char* s, cardState fallback);
/** @copydoc parseOrDefault(const char*, logicCardType) */
combineMode parseOrDefault(const char* s, combineMode fallback);



