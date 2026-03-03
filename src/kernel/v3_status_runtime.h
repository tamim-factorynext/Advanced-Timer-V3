/*
File: src/kernel/v3_status_runtime.h
Purpose: Declares the v3 status runtime module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.cpp
- src/kernel/v3_status_runtime.cpp
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#pragma once

#include "kernel/card_model.h"

bool isMissionRunning(logicCardType type, cardState state);
bool isMissionFinished(logicCardType type, cardState state);
bool isMissionStopped(logicCardType type, cardState state);

