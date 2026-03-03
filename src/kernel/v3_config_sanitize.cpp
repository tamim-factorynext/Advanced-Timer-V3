/*
File: src/kernel/v3_config_sanitize.cpp
Purpose: Implements the v3 config sanitize module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/kernel/legacy_card_profile.cpp
- src/kernel/v3_config_sanitize.h
Flow Hook:
- Kernel scan cycle and card runtime evaluation.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "kernel/v3_config_sanitize.h"

void sanitizeConfigCardRuntimeFields(LogicCard& card) {
  // Runtime-mutable fields are never accepted from persisted config payloads.
  card.commandState = false;
  card.actualState = false;
  card.edgePulse = false;
  card.liveValue = 0;
  card.repeatCounter = 0;

  switch (card.type) {
    case DigitalInput:
      card.startOnMs = 0;
      card.startOffMs = 0;
      card.state = State_DI_Idle;
      break;
    case DigitalOutput:
    case SoftIO:
      card.startOnMs = 0;
      card.startOffMs = 0;
      card.state = State_DO_Idle;
      break;
    case AnalogInput:
      card.state = State_AI_Streaming;
      break;
    case MathCard:
      card.state = State_None;
      break;
    case RtcCard:
      card.startOnMs = 0;
      card.state = State_None;
      break;
    default:
      break;
  }
}

void sanitizeConfigCardsRuntimeFields(LogicCard* cards, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    sanitizeConfigCardRuntimeFields(cards[i]);
  }
}

