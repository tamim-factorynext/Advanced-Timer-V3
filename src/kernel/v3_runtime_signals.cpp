#include "kernel/v3_runtime_signals.h"

V3RuntimeSignal makeRuntimeSignal(const RuntimeCardMeta& meta,
                                  const V3RuntimeStoreView& store) {
  V3RuntimeSignal signal = {};
  signal.type = meta.type;

  switch (meta.type) {
    case DigitalInput: {
      if (meta.index >= store.diCount) return signal;
      const V3DiRuntimeState& runtime = store.di[meta.index];
      signal.state = runtime.state;
      signal.logicalState = runtime.logicalState;
      signal.physicalState = runtime.physicalState;
      signal.triggerFlag = runtime.triggerFlag;
      signal.currentValue = runtime.currentValue;
      return signal;
    }
    case DigitalOutput: {
      if (meta.index >= store.dOutCount) return signal;
      const V3DoRuntimeState& runtime = store.dOut[meta.index];
      signal.state = runtime.state;
      signal.logicalState = runtime.logicalState;
      signal.physicalState = runtime.physicalState;
      signal.triggerFlag = runtime.triggerFlag;
      signal.currentValue = runtime.currentValue;
      return signal;
    }
    case AnalogInput: {
      if (meta.index >= store.aiCount) return signal;
      const V3AiRuntimeState& runtime = store.ai[meta.index];
      signal.state = runtime.state;
      signal.logicalState = false;
      signal.physicalState = false;
      signal.triggerFlag = false;
      signal.currentValue = runtime.currentValue;
      return signal;
    }
    case SoftIO: {
      if (meta.index >= store.sioCount) return signal;
      const V3SioRuntimeState& runtime = store.sio[meta.index];
      signal.state = runtime.state;
      signal.logicalState = runtime.logicalState;
      signal.physicalState = runtime.physicalState;
      signal.triggerFlag = runtime.triggerFlag;
      signal.currentValue = runtime.currentValue;
      return signal;
    }
    case MathCard: {
      if (meta.index >= store.mathCount) return signal;
      const V3MathRuntimeState& runtime = store.math[meta.index];
      signal.state = runtime.state;
      signal.logicalState = runtime.logicalState;
      signal.physicalState = runtime.physicalState;
      signal.triggerFlag = runtime.triggerFlag;
      signal.currentValue = runtime.currentValue;
      return signal;
    }
    case RtcCard: {
      if (meta.index >= store.rtcCount) return signal;
      const V3RtcRuntimeState& runtime = store.rtc[meta.index];
      signal.state = runtime.state;
      signal.logicalState = runtime.logicalState;
      signal.physicalState = runtime.physicalState;
      signal.triggerFlag = runtime.triggerFlag;
      signal.currentValue = runtime.currentValue;
      return signal;
    }
    default:
      return signal;
  }
}

void refreshRuntimeSignalsFromRuntime(const RuntimeCardMeta* cardsMeta,
                                      const V3RuntimeStoreView& store,
                                      V3RuntimeSignal* out, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    out[i] = makeRuntimeSignal(cardsMeta[i], store);
  }
}

void refreshRuntimeSignalAt(const RuntimeCardMeta* cardsMeta,
                            const V3RuntimeStoreView& store,
                            V3RuntimeSignal* out, uint8_t count,
                            uint8_t cardId) {
  if (cardId >= count) return;
  out[cardId] = makeRuntimeSignal(cardsMeta[cardId], store);
}
