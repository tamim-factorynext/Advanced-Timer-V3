#include "kernel/v3_config_sanitize.h"

void sanitizeConfigCardRuntimeFields(LogicCard& card) {
  // Runtime-mutable fields are never accepted from persisted config payloads.
  card.logicalState = false;
  card.physicalState = false;
  card.triggerFlag = false;
  card.currentValue = 0;
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
