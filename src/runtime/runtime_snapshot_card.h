#pragma once

#include <stdint.h>

#include "kernel/card_model.h"

struct RuntimeSnapshotCard {
  uint8_t id;
  logicCardType type;
  uint8_t index;
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  cardState state;
  cardMode mode;
  uint32_t currentValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  bool setResult;
  bool resetResult;
};
