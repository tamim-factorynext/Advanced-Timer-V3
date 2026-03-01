#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct V3RuntimeSignal {
  logicCardType type;
  cardState state;
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
};

V3RuntimeSignal makeRuntimeSignal(const LogicCard& card);
void refreshRuntimeSignalsFromCards(const LogicCard* cards, V3RuntimeSignal* out,
                                    uint8_t count);
void refreshRuntimeSignalAt(const LogicCard* cards, V3RuntimeSignal* out,
                           uint8_t count, uint8_t cardId);

