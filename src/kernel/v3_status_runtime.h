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

/**
 * @brief Reports whether a card state indicates mission-in-progress.
 * @details Meaning depends on card family state-machine semantics.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
bool isMissionRunning(logicCardType type, cardState state);
/**
 * @brief Reports whether a card state indicates mission-finished condition.
 * @details Used for condition-evaluation operators mapped to mission lifecycle.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
bool isMissionFinished(logicCardType type, cardState state);
/**
 * @brief Reports whether a card state indicates mission-stopped condition.
 * @details Used for condition-evaluation operators mapped to mission lifecycle.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 */
bool isMissionStopped(logicCardType type, cardState state);

