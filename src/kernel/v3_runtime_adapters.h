#pragma once

#include "kernel/card_model.h"
#include "kernel/v3_ai_runtime.h"
#include "kernel/v3_di_runtime.h"
#include "kernel/v3_do_runtime.h"
#include "kernel/v3_math_runtime.h"
#include "kernel/v3_rtc_runtime.h"
#include "kernel/v3_sio_runtime.h"

V3DiRuntimeConfig makeDiRuntimeConfig(const LogicCard& card);
V3DiRuntimeState makeDiRuntimeState(const LogicCard& card);
void applyDiRuntimeState(LogicCard& card, const V3DiRuntimeState& runtime);

V3AiRuntimeConfig makeAiRuntimeConfig(const LogicCard& card);
V3AiRuntimeState makeAiRuntimeState(const LogicCard& card);
void applyAiRuntimeState(LogicCard& card, const V3AiRuntimeState& runtime);

V3DoRuntimeConfig makeDoRuntimeConfig(const LogicCard& card);
V3DoRuntimeState makeDoRuntimeState(const LogicCard& card);
void applyDoRuntimeState(LogicCard& card, const V3DoRuntimeState& runtime);

V3SioRuntimeConfig makeSioRuntimeConfig(const LogicCard& card);
V3SioRuntimeState makeSioRuntimeState(const LogicCard& card);
void applySioRuntimeState(LogicCard& card, const V3SioRuntimeState& runtime);

V3MathRuntimeConfig makeMathRuntimeConfig(const LogicCard& card);
V3MathRuntimeState makeMathRuntimeState(const LogicCard& card);
void applyMathRuntimeState(LogicCard& card, const V3MathRuntimeState& runtime);

V3RtcRuntimeConfig makeRtcRuntimeConfig(const LogicCard& card);
V3RtcRuntimeState makeRtcRuntimeState(const LogicCard& card);
void applyRtcRuntimeState(LogicCard& card, const V3RtcRuntimeState& runtime);

