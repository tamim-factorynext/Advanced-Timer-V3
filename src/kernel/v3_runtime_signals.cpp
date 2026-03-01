#include "kernel/v3_runtime_signals.h"

V3RuntimeSignal makeRuntimeSignal(const LogicCard& card) {
  V3RuntimeSignal signal = {};
  signal.type = card.type;
  signal.state = card.state;
  signal.logicalState = card.logicalState;
  signal.physicalState = card.physicalState;
  signal.triggerFlag = card.triggerFlag;
  signal.currentValue = card.currentValue;
  return signal;
}

void refreshRuntimeSignalsFromCards(const LogicCard* cards, V3RuntimeSignal* out,
                                    uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    out[i] = makeRuntimeSignal(cards[i]);
  }
}

void refreshRuntimeSignalAt(const LogicCard* cards, V3RuntimeSignal* out,
                           uint8_t count, uint8_t cardId) {
  if (cardId >= count) return;
  out[cardId] = makeRuntimeSignal(cards[cardId]);
}

