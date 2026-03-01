#include "kernel/v3_runtime_store.h"

#include "kernel/v3_runtime_adapters.h"

V3DiRuntimeState* runtimeDiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store) {
  if (index >= store.diCount) return nullptr;
  return &store.di[index];
}

V3DoRuntimeState* runtimeDoStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store) {
  if (index >= store.dOutCount) return nullptr;
  return &store.dOut[index];
}

V3AiRuntimeState* runtimeAiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store) {
  if (index >= store.aiCount) return nullptr;
  return &store.ai[index];
}

V3SioRuntimeState* runtimeSioStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store) {
  if (index >= store.sioCount) return nullptr;
  return &store.sio[index];
}

V3MathRuntimeState* runtimeMathStateAt(uint8_t index,
                                       const V3RuntimeStoreView& store) {
  if (index >= store.mathCount) return nullptr;
  return &store.math[index];
}

V3RtcRuntimeState* runtimeRtcStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store) {
  if (index >= store.rtcCount) return nullptr;
  return &store.rtc[index];
}

void syncRuntimeStoreFromTypedCards(const LogicCard* cards,
                                    const V3CardConfig* typedCards,
                                    uint8_t cardCount,
                                    const V3RuntimeStoreView& store) {
  if (cards == nullptr || typedCards == nullptr) return;
  for (uint8_t i = 0; i < cardCount; ++i) {
    const LogicCard& card = cards[i];
    const V3CardConfig& typed = typedCards[i];
    switch (typed.family) {
      case V3CardFamily::DI: {
        V3DiRuntimeState* state = runtimeDiStateAt(typed.di.channel, store);
        if (state != nullptr) *state = makeDiRuntimeState(card);
        break;
      }
      case V3CardFamily::DO: {
        V3DoRuntimeState* state = runtimeDoStateAt(typed.dout.channel, store);
        if (state != nullptr) *state = makeDoRuntimeState(card);
        break;
      }
      case V3CardFamily::AI: {
        V3AiRuntimeState* state = runtimeAiStateAt(typed.ai.channel, store);
        if (state != nullptr) *state = makeAiRuntimeState(card);
        break;
      }
      case V3CardFamily::SIO: {
        V3SioRuntimeState* state = runtimeSioStateAt(card.index, store);
        if (state != nullptr) *state = makeSioRuntimeState(card);
        break;
      }
      case V3CardFamily::MATH: {
        V3MathRuntimeState* state = runtimeMathStateAt(card.index, store);
        if (state != nullptr) *state = makeMathRuntimeState(card);
        break;
      }
      case V3CardFamily::RTC: {
        V3RtcRuntimeState* state = runtimeRtcStateAt(card.index, store);
        if (state != nullptr) *state = makeRtcRuntimeState(card);
        break;
      }
      default:
        break;
    }
  }
}

void mirrorRuntimeStoreCardToLegacyByTyped(LogicCard& card,
                                           const V3CardConfig& typedCard,
                                           const V3RuntimeStoreView& store) {
  switch (typedCard.family) {
    case V3CardFamily::DI: {
      const V3DiRuntimeState* state = runtimeDiStateAt(typedCard.di.channel, store);
      if (state != nullptr) applyDiRuntimeState(card, *state);
      break;
    }
    case V3CardFamily::DO: {
      const V3DoRuntimeState* state =
          runtimeDoStateAt(typedCard.dout.channel, store);
      if (state != nullptr) applyDoRuntimeState(card, *state);
      break;
    }
    case V3CardFamily::AI: {
      const V3AiRuntimeState* state = runtimeAiStateAt(typedCard.ai.channel, store);
      if (state != nullptr) applyAiRuntimeState(card, *state);
      break;
    }
    case V3CardFamily::SIO: {
      const V3SioRuntimeState* state = runtimeSioStateAt(card.index, store);
      if (state != nullptr) applySioRuntimeState(card, *state);
      break;
    }
    case V3CardFamily::MATH: {
      const V3MathRuntimeState* state = runtimeMathStateAt(card.index, store);
      if (state != nullptr) applyMathRuntimeState(card, *state);
      break;
    }
    case V3CardFamily::RTC: {
      const V3RtcRuntimeState* state = runtimeRtcStateAt(card.index, store);
      if (state != nullptr) applyRtcRuntimeState(card, *state);
      break;
    }
    default:
      break;
  }
}
