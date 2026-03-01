#pragma once

#include "kernel/card_model.h"

inline uint32_t legacyDiDebounceMs(const LogicCard& card) {
  return card.setting1;
}

inline void setLegacyDiDebounceMs(LogicCard& card, uint32_t value) {
  card.setting1 = value;
}

inline uint32_t legacyDoDelayBeforeOnMs(const LogicCard& card) {
  return card.setting1;
}

inline uint32_t legacyDoOnDurationMs(const LogicCard& card) {
  return card.setting2;
}

inline uint32_t legacyDoRepeatCount(const LogicCard& card) {
  return card.setting3;
}

inline void setLegacyDoDelayBeforeOnMs(LogicCard& card, uint32_t value) {
  card.setting1 = value;
}

inline void setLegacyDoOnDurationMs(LogicCard& card, uint32_t value) {
  card.setting2 = value;
}

inline void setLegacyDoRepeatCount(LogicCard& card, uint32_t value) {
  card.setting3 = value;
}

inline uint32_t legacyAiInputMin(const LogicCard& card) {
  return card.setting1;
}

inline uint32_t legacyAiInputMax(const LogicCard& card) {
  return card.setting2;
}

inline uint32_t legacyAiOutputMin(const LogicCard& card) {
  return card.startOnMs;
}

inline uint32_t legacyAiOutputMax(const LogicCard& card) {
  return card.startOffMs;
}

inline uint32_t legacyAiAlphaX1000(const LogicCard& card) {
  return card.setting3;
}

inline void setLegacyAiInputMin(LogicCard& card, uint32_t value) {
  card.setting1 = value;
}

inline void setLegacyAiInputMax(LogicCard& card, uint32_t value) {
  card.setting2 = value;
}

inline void setLegacyAiOutputMin(LogicCard& card, uint32_t value) {
  card.startOnMs = value;
}

inline void setLegacyAiOutputMax(LogicCard& card, uint32_t value) {
  card.startOffMs = value;
}

inline void setLegacyAiAlphaX1000(LogicCard& card, uint32_t value) {
  card.setting3 = value;
}

inline uint32_t legacyMathInputA(const LogicCard& card) {
  return card.setting1;
}

inline uint32_t legacyMathInputB(const LogicCard& card) {
  return card.setting2;
}

inline uint32_t legacyMathFallbackValue(const LogicCard& card) {
  return card.setting3;
}

inline uint32_t legacyMathClampMin(const LogicCard& card) {
  return card.startOnMs;
}

inline uint32_t legacyMathClampMax(const LogicCard& card) {
  return card.startOffMs;
}

inline void setLegacyMathInputA(LogicCard& card, uint32_t value) {
  card.setting1 = value;
}

inline void setLegacyMathInputB(LogicCard& card, uint32_t value) {
  card.setting2 = value;
}

inline void setLegacyMathFallbackValue(LogicCard& card, uint32_t value) {
  card.setting3 = value;
}

inline void setLegacyMathClampMin(LogicCard& card, uint32_t value) {
  card.startOnMs = value;
}

inline void setLegacyMathClampMax(LogicCard& card, uint32_t value) {
  card.startOffMs = value;
}

inline uint32_t legacyRtcTriggerDurationMs(const LogicCard& card) {
  return card.setting1;
}

inline void setLegacyRtcTriggerDurationMs(LogicCard& card, uint32_t value) {
  card.setting1 = value;
}
