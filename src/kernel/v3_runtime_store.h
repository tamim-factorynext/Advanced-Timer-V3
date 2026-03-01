#pragma once

#include <stdint.h>

#include "kernel/card_model.h"
#include "kernel/v3_card_types.h"
#include "kernel/v3_ai_runtime.h"
#include "kernel/v3_di_runtime.h"
#include "kernel/v3_do_runtime.h"
#include "kernel/v3_math_runtime.h"
#include "kernel/v3_rtc_runtime.h"
#include "kernel/v3_sio_runtime.h"

struct V3RuntimeStoreView {
  V3DiRuntimeState* di;
  uint8_t diCount;
  V3DoRuntimeState* dOut;
  uint8_t dOutCount;
  V3AiRuntimeState* ai;
  uint8_t aiCount;
  V3SioRuntimeState* sio;
  uint8_t sioCount;
  V3MathRuntimeState* math;
  uint8_t mathCount;
  V3RtcRuntimeState* rtc;
  uint8_t rtcCount;
};

void syncRuntimeStoreFromTypedCards(const LogicCard* cards,
                                    const V3CardConfig* typedCards,
                                    uint8_t cardCount,
                                    const V3RuntimeStoreView& store);
void mirrorRuntimeStoreCardToLegacyByTyped(LogicCard& card,
                                           const V3CardConfig& typedCard,
                                           const V3RuntimeStoreView& store);

V3DiRuntimeState* runtimeDiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store);
V3DoRuntimeState* runtimeDoStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store);
V3AiRuntimeState* runtimeAiStateAt(uint8_t index,
                                   const V3RuntimeStoreView& store);
V3SioRuntimeState* runtimeSioStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store);
V3MathRuntimeState* runtimeMathStateAt(uint8_t index,
                                       const V3RuntimeStoreView& store);
V3RtcRuntimeState* runtimeRtcStateAt(uint8_t index,
                                     const V3RuntimeStoreView& store);
