#include "kernel/v3_runtime_store.h"

#include "kernel/v3_runtime_adapters.h"

V3DiRuntimeState* runtimeDiStateForCard(const LogicCard& card,
                                        const V3RuntimeStoreView& store) {
  if (card.type != DigitalInput) return nullptr;
  return runtimeDiStateAt(card.index, store);
}

V3DiRuntimeState* runtimeDiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store) {
  if (index >= store.diCount) return nullptr;
  return &store.di[index];
}

V3DoRuntimeState* runtimeDoStateForCard(const LogicCard& card,
                                        const V3RuntimeStoreView& store) {
  if (card.type != DigitalOutput) return nullptr;
  return runtimeDoStateAt(card.index, store);
}

V3DoRuntimeState* runtimeDoStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store) {
  if (index >= store.dOutCount) return nullptr;
  return &store.dOut[index];
}

V3AiRuntimeState* runtimeAiStateForCard(const LogicCard& card,
                                        const V3RuntimeStoreView& store) {
  if (card.type != AnalogInput) return nullptr;
  return runtimeAiStateAt(card.index, store);
}

V3AiRuntimeState* runtimeAiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store) {
  if (index >= store.aiCount) return nullptr;
  return &store.ai[index];
}

V3SioRuntimeState* runtimeSioStateForCard(const LogicCard& card,
                                          const V3RuntimeStoreView& store) {
  if (card.type != SoftIO) return nullptr;
  return runtimeSioStateAt(card.index, store);
}

V3SioRuntimeState* runtimeSioStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store) {
  if (index >= store.sioCount) return nullptr;
  return &store.sio[index];
}

V3MathRuntimeState* runtimeMathStateForCard(const LogicCard& card,
                                            const V3RuntimeStoreView& store) {
  if (card.type != MathCard) return nullptr;
  return runtimeMathStateAt(card.index, store);
}

V3MathRuntimeState* runtimeMathStateAt(uint8_t index,
                                       const V3RuntimeStoreView& store) {
  if (index >= store.mathCount) return nullptr;
  return &store.math[index];
}

V3RtcRuntimeState* runtimeRtcStateForCard(const LogicCard& card,
                                          const V3RuntimeStoreView& store) {
  if (card.type != RtcCard) return nullptr;
  return runtimeRtcStateAt(card.index, store);
}

V3RtcRuntimeState* runtimeRtcStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store) {
  if (index >= store.rtcCount) return nullptr;
  return &store.rtc[index];
}

void syncRuntimeStoreFromCards(const LogicCard* cards, uint8_t cardCount,
                               const V3RuntimeStoreView& store) {
  for (uint8_t i = 0; i < cardCount; ++i) {
    const LogicCard& card = cards[i];
    switch (card.type) {
      case DigitalInput: {
        V3DiRuntimeState* state = runtimeDiStateForCard(card, store);
        if (state != nullptr) *state = makeDiRuntimeState(card);
        break;
      }
      case DigitalOutput: {
        V3DoRuntimeState* state = runtimeDoStateForCard(card, store);
        if (state != nullptr) *state = makeDoRuntimeState(card);
        break;
      }
      case AnalogInput: {
        V3AiRuntimeState* state = runtimeAiStateForCard(card, store);
        if (state != nullptr) *state = makeAiRuntimeState(card);
        break;
      }
      case SoftIO: {
        V3SioRuntimeState* state = runtimeSioStateForCard(card, store);
        if (state != nullptr) *state = makeSioRuntimeState(card);
        break;
      }
      case MathCard: {
        V3MathRuntimeState* state = runtimeMathStateForCard(card, store);
        if (state != nullptr) *state = makeMathRuntimeState(card);
        break;
      }
      case RtcCard: {
        V3RtcRuntimeState* state = runtimeRtcStateForCard(card, store);
        if (state != nullptr) *state = makeRtcRuntimeState(card);
        break;
      }
      default:
        break;
    }
  }
}

void mirrorRuntimeStoreCardToLegacy(LogicCard& card,
                                    const V3RuntimeStoreView& store) {
  switch (card.type) {
    case DigitalInput: {
      const V3DiRuntimeState* state = runtimeDiStateForCard(card, store);
      if (state != nullptr) applyDiRuntimeState(card, *state);
      break;
    }
    case DigitalOutput: {
      const V3DoRuntimeState* state = runtimeDoStateForCard(card, store);
      if (state != nullptr) applyDoRuntimeState(card, *state);
      break;
    }
    case AnalogInput: {
      const V3AiRuntimeState* state = runtimeAiStateForCard(card, store);
      if (state != nullptr) applyAiRuntimeState(card, *state);
      break;
    }
    case SoftIO: {
      const V3SioRuntimeState* state = runtimeSioStateForCard(card, store);
      if (state != nullptr) applySioRuntimeState(card, *state);
      break;
    }
    case MathCard: {
      const V3MathRuntimeState* state = runtimeMathStateForCard(card, store);
      if (state != nullptr) applyMathRuntimeState(card, *state);
      break;
    }
    case RtcCard: {
      const V3RtcRuntimeState* state = runtimeRtcStateForCard(card, store);
      if (state != nullptr) applyRtcRuntimeState(card, *state);
      break;
    }
    default:
      break;
  }
}
