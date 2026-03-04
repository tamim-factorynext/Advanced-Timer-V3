/*
File: src/runtime/runtime_snapshot_card.h
Purpose: Declares the runtime snapshot card module interface and data contracts.

Responsibilities:
- Define stable types/functions consumed by other modules.
- Keep cross-module contract changes explicit and reviewable.

Used By:
- src/kernel/kernel_service.h
- src/main.cpp
- src/portal/portal_service.h
- (+ more call sites)
Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-legacy-v3.md where applicable.
*/
#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

/**
 * @brief Per-card runtime snapshot row exported from kernel scan loop.
 * @details Represents transport-friendly state projection for one configured card.
 * @par Field Semantics
 * - `id/type/index`: card identity and family-local index.
 * - `commandState/actualState/edgePulse`: boolean runtime state channels.
 * - `state/mode`: card runtime state-machine and configured mode value.
 * - `liveValue`: current numeric value channel.
 * - `startOnMs/startOffMs/repeatCounter`: timing/cycle telemetry for timed families.
 * - `turnOnConditionMet/turnOffConditionMet`: latest condition evaluation outcomes.
 * @par Used By
 * - src/kernel/kernel_service.cpp
 * - src/main.cpp
 * - src/portal/portal_service.cpp
 */
struct RuntimeSnapshotCard {
  uint8_t id;
  logicCardType type;
  uint8_t index;
  bool commandState;
  bool actualState;
  bool edgePulse;
  cardState state;
  cardMode mode;
  uint32_t liveValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  bool turnOnConditionMet;
  bool turnOffConditionMet;
};




