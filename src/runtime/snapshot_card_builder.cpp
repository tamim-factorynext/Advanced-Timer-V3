#include "runtime/snapshot_card_builder.h"

RuntimeSnapshotCard buildRuntimeSnapshotCard(const RuntimeCardMeta& meta,
                                             const V3RuntimeStoreView& store) {
  RuntimeSnapshotCard out = {};
  out.id = meta.id;
  out.type = meta.type;
  out.index = meta.index;
  out.mode = meta.mode;

  switch (meta.type) {
    case DigitalInput: {
      const V3DiRuntimeState* runtime = runtimeDiStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.logicalState = runtime->logicalState;
        out.physicalState = runtime->physicalState;
        out.triggerFlag = runtime->triggerFlag;
        out.state = runtime->state;
        out.currentValue = runtime->currentValue;
        out.startOnMs = runtime->startOnMs;
        out.startOffMs = runtime->startOffMs;
        out.repeatCounter = runtime->repeatCounter;
      }
      break;
    }
    case DigitalOutput: {
      const V3DoRuntimeState* runtime = runtimeDoStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.logicalState = runtime->logicalState;
        out.physicalState = runtime->physicalState;
        out.triggerFlag = runtime->triggerFlag;
        out.state = runtime->state;
        out.currentValue = runtime->currentValue;
        out.startOnMs = runtime->startOnMs;
        out.startOffMs = runtime->startOffMs;
        out.repeatCounter = runtime->repeatCounter;
      }
      break;
    }
    case AnalogInput: {
      const V3AiRuntimeState* runtime = runtimeAiStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.state = runtime->state;
        out.mode = runtime->mode;
        out.currentValue = runtime->currentValue;
      }
      break;
    }
    case SoftIO: {
      const V3SioRuntimeState* runtime = runtimeSioStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.logicalState = runtime->logicalState;
        out.physicalState = runtime->physicalState;
        out.triggerFlag = runtime->triggerFlag;
        out.state = runtime->state;
        out.currentValue = runtime->currentValue;
        out.startOnMs = runtime->startOnMs;
        out.startOffMs = runtime->startOffMs;
        out.repeatCounter = runtime->repeatCounter;
      }
      break;
    }
    case MathCard: {
      const V3MathRuntimeState* runtime = runtimeMathStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.logicalState = runtime->logicalState;
        out.physicalState = runtime->physicalState;
        out.triggerFlag = runtime->triggerFlag;
        out.state = runtime->state;
        out.currentValue = runtime->currentValue;
      }
      break;
    }
    case RtcCard: {
      const V3RtcRuntimeState* runtime = runtimeRtcStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.logicalState = runtime->logicalState;
        out.physicalState = runtime->physicalState;
        out.triggerFlag = runtime->triggerFlag;
        out.state = runtime->state;
        out.mode = runtime->mode;
        out.currentValue = runtime->currentValue;
        out.startOnMs = runtime->triggerStartMs;
      }
      break;
    }
    default:
      break;
  }

  return out;
}

void buildRuntimeSnapshotCards(const RuntimeCardMeta* meta, uint8_t count,
                               const V3RuntimeStoreView& store,
                               RuntimeSnapshotCard* outCards) {
  for (uint8_t i = 0; i < count; ++i) {
    outCards[i] = buildRuntimeSnapshotCard(meta[i], store);
  }
}
