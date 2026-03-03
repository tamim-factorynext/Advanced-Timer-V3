/*
File: src/runtime/snapshot_card_builder.cpp
Purpose: Implements the snapshot card builder module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/runtime/runtime_card_meta.cpp
- src/kernel/v3_runtime_store.cpp

Flow Hook:
- Runtime snapshot shaping and cross-module data projection.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
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
        out.commandState = runtime->commandState;
        out.actualState = runtime->actualState;
        out.edgePulse = runtime->edgePulse;
        out.state = runtime->state;
        out.liveValue = runtime->liveValue;
        out.startOnMs = runtime->startOnMs;
        out.startOffMs = runtime->startOffMs;
        out.repeatCounter = runtime->repeatCounter;
      }
      break;
    }
    case DigitalOutput: {
      const V3DoRuntimeState* runtime = runtimeDoStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.commandState = runtime->commandState;
        out.actualState = runtime->actualState;
        out.edgePulse = runtime->edgePulse;
        out.state = runtime->state;
        out.liveValue = runtime->liveValue;
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
        out.liveValue = runtime->liveValue;
      }
      break;
    }
    case SoftIO: {
      const V3SioRuntimeState* runtime = runtimeSioStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.commandState = runtime->commandState;
        out.actualState = runtime->actualState;
        out.edgePulse = runtime->edgePulse;
        out.state = runtime->state;
        out.liveValue = runtime->liveValue;
        out.startOnMs = runtime->startOnMs;
        out.startOffMs = runtime->startOffMs;
        out.repeatCounter = runtime->repeatCounter;
      }
      break;
    }
    case MathCard: {
      const V3MathRuntimeState* runtime = runtimeMathStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.commandState = runtime->commandState;
        out.actualState = runtime->actualState;
        out.edgePulse = runtime->edgePulse;
        out.state = runtime->state;
        out.liveValue = runtime->liveValue;
      }
      break;
    }
    case RtcCard: {
      const V3RtcRuntimeState* runtime = runtimeRtcStateAt(meta.index, store);
      if (runtime != nullptr) {
        out.commandState = runtime->commandState;
        out.edgePulse = runtime->edgePulse;
        out.state = runtime->state;
        out.mode = runtime->mode;
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

