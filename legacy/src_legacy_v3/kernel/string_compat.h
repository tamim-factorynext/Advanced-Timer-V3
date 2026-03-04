/*
File: src/kernel/string_compat.h
Purpose: Declares the string compat module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/legacy_config_validator.h
- src/kernel/legacy_v3_typed_card_parser.h
- src/storage/legacy_config_lifecycle.h
- (+ more call sites)
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
using String = std::string;
#endif


