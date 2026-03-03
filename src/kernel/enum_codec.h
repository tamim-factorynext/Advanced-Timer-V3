/*
File: src/kernel/enum_codec.h
Purpose: Declares the enum codec module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src\kernel\enum_codec.cpp
- src\kernel\enum_codec.h
- src\kernel\legacy_card_profile.cpp
- src\kernel\legacy_config_validator.cpp
- (+ more call sites)

Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "control/command_dto.h"
#include "kernel/card_model.h"

const char* toString(logicCardType value);
const char* toString(logicOperator value);
const char* toString(cardMode value);
const char* toString(cardState value);
const char* toString(combineMode value);
const char* toString(engineMode value);
const char* toString(inputSourceMode value);

bool tryParseLogicCardType(const char* s, logicCardType& out);
bool tryParseLogicOperator(const char* s, logicOperator& out);
bool tryParseCardMode(const char* s, cardMode& out);
bool tryParseCardState(const char* s, cardState& out);
bool tryParseCombineMode(const char* s, combineMode& out);

logicCardType parseOrDefault(const char* s, logicCardType fallback);
logicOperator parseOrDefault(const char* s, logicOperator fallback);
cardMode parseOrDefault(const char* s, cardMode fallback);
cardState parseOrDefault(const char* s, cardState fallback);
combineMode parseOrDefault(const char* s, combineMode fallback);

