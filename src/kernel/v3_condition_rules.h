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

bool parseV3CardTypeToken(const char* cardType, logicCardType& outType);
bool isV3FieldAllowedForSourceType(logicCardType sourceType, const char* field);
bool isV3OperatorAllowedForField(const char* field, const char* op);
