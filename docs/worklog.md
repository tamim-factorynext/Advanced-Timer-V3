# Worklog

Status: Canonical consolidated session log for rewrite documentation and implementation tracking.

Naming Baseline (2026-02-28): Rewrite track is now `V3`; frozen PoC code/contracts are `V2`.

## 2026-02-26

### Session Summary

Today we converted planning into implementation-grade documentation for V2.

### Completed

- Updated `requirements-v3-contract.md` with final decisions:
  - MATH uses arithmetic operators only (no comparison operators).
  - RTC switched to field-based schedule (`year/month/day/hour/minute/second/weekday`) with wildcard semantics.
  - RTC uses `triggerDuration` for asserted output window.
  - DO/SIO state comparison model finalized:
    - only `missionState` for DO/SIO
    - states: `IDLE | ACTIVE | FINISHED`
    - STATE comparison operator: `EQ` only
    - non-match => `false`
  - Restore model simplified to single rollback + factory:
    - `LKG` (last known good)
    - `FACTORY`

- Created/updated docs:
  - `docs/schema-v3.md` (schema draft aligned with above decisions)
  - `docs/acceptance-matrix-v3.md` (acceptance coverage expanded)
  - `docs/poc-gap-log-v3.md` (PoC->V3 gap tracking updated)
  - `docs/api-contract-v3.md` (new, payload-level API freeze draft)

- Gap status:
  - GAP-005 resolved (STATE condition model)
  - GAP-006 resolved (`emaAlpha` frozen to centiunits `0..100` for `0.00..1.00`)
  - GAP-008 resolved (restore sources frozen to `LKG|FACTORY`)
  - GAP-007 drafted as resolved via `docs/api-contract-v3.md`

### Current Blocker

- Unresolved merge markers exist in:
  - `docs/acceptance-matrix-v3.md` (lines around 112-116)
- This must be cleaned first before further edits.

### Exact Start Plan For Next Session

1. Resolve merge markers in `docs/acceptance-matrix-v3.md`.
2. Re-run consistency pass across:
   - `requirements-v3-contract.md`
   - `docs/schema-v3.md`
   - `docs/api-contract-v3.md`
   - `docs/poc-gap-log-v3.md`
   - `docs/acceptance-matrix-v3.md`
3. Confirm final gap table status (all intended gaps closed, or explicitly deferred).
4. Start implementation phase:
   - validator rules (`V-CFG-*`)
   - API envelope validation and error mapping
   - acceptance test scaffolding for `AT-API-*` and `AT-CFG-006`

### Notes

- Keep `RUN_SLOW` rejected in V2 API/command validation.
- Keep restore source validation strict: only `LKG` and `FACTORY`.
- Keep STATE comparison strict: DO/SIO only, `EQ` only.

## 2026-02-28

### Session Summary

Established a minimal "vibe-safe" documentation workflow and aligned V2 docs with the decision to keep `lastEvalUs` as runtime observability metadata.

### Completed

- Added `docs/decisions.md`:
  - Minimal decision-entry template.
  - Workflow rules for contract-first changes.
  - `DEC-0001` documenting `lastEvalUs` as per-card evaluation timing metric.

- Updated `docs/api-contract-v3.md`:
  - Added `docs/decisions.md` in related docs.
  - Clarified `lastEvalUs` semantics:
    - microsecond unit
    - non-negative integer
    - runtime-only (not required in config payloads)

- Updated process docs:
  - `README.md` Section 18.4 now requires decision-log updates for behavior/API/validation changes.
  - `requirements-v3-contract.md` now includes decision-log requirement and lists `docs/decisions.md` as a required artifact.

- Cleaned unresolved merge markers in `docs/acceptance-matrix-v3.md`.

### Next Session Starting Point

1. Add validator tests that assert `lastEvalUs` handling as runtime observability metadata.
2. Ensure portal/runtime UI rendering treats `lastEvalUs` as optional forward-compatible telemetry.

## 2026-02-28 (Hardware Profile Addendum)

### Session Summary

Added V2 hardware-profile gating contract so build targets can define available IO families/channels and optional capabilities (for example RTC) without changing card logic semantics.

### Completed

- Updated `platformio.ini` with hardware-profile build flag scaffold:
  - profile name
  - per-family card gates
  - RTC gate
  - AI backend selection
  - optional IO pin list macros

- Added `docs/hardware-profile-v3.md`:
  - build-time hardware profile model
  - empty-array semantics for unavailable IO families
  - RTC gating rules
  - backend abstraction contract
  - plugin boundary for remote/protocol-specific IO

- Updated `requirements-v3-contract.md`:
  - Added Section `6.4 Build-Time Hardware Profile Contract`
  - Added `docs/hardware-profile-v3.md` to required artifacts

- Updated `docs/schema-v3.md`:
  - Linked hardware profile contract
  - Added validation IDs `V-CFG-017..019` for profile gates/capacity/RTC support

- Updated `docs/acceptance-matrix-v3.md`:
  - Added hardware-profile acceptance tests `AT-HW-001..004`

## 2026-02-28 (Capacity Model Clarification)

### Session Summary

Finalized the family-capacity model: every card family (physical and virtual) is compile-time capacity-bound by hardware profile, including RTC as schedule-alarm channels.

### Completed

- Updated `docs/hardware-profile-v3.md`:
  - Added explicit compile-time capacities per family.
  - Clarified RTC as schedule-based alarm channel capacity (`0..N` by profile/hardware design).
  - Clarified that all families can be `0` and are profile-optional.

- Updated `requirements-v3-contract.md`:
  - Added compile-time capacity language in hardware profile requirements.
  - Clarified RTC section with alarm-channel capacity gating.

- Updated `docs/schema-v3.md`:
  - Added compile-time family-capacity statement for all families.

- Updated `docs/acceptance-matrix-v3.md`:
  - Added `AT-HW-007` for RTC alarm-capacity enforcement.

- Updated `docs/decisions.md`:
  - Added `DEC-0002` to record this capacity-model decision.

## 2026-02-28 (Phase 0 Metrics Instrumentation Start)

### Session Summary

Started execution of kickoff plan Phase 0 instrumentation tasks by adding runtime telemetry fields required for measurable regression tracking.

### Completed

- Updated `src/main.cpp` runtime snapshot telemetry:
  - Added scan metrics:
    - `scanLastUs`
    - `scanMaxUs`
  - Added kernel command queue metrics:
    - `queueDepth`
    - `queueHighWaterMark`
    - `queueCapacity`
  - Added command processing latency metrics:
    - `commandLatencyLastUs`
    - `commandLatencyMaxUs`

- Added runtime counters/state needed to populate these metrics:
  - max complete scan duration tracking
  - queue depth/high-water tracking on enqueue/process
  - enqueue timestamp + enqueue-to-apply latency tracking

### Notes

- Local compile verification is currently blocked in this shell because `platformio` / `pio` CLI is not available on PATH.

## 2026-02-28 (Phase 0 Metrics Instrumentation Continued)

### Completed

- Extended runtime snapshot metrics in `src/main.cpp` with explicit timing-budget visibility:
  - `metrics.scanBudgetUs`
  - `metrics.scanOverrunLast`
  - `metrics.scanOverrunCount`

- Added overrun tracking in runtime engine:
  - budget derived from active scan interval
  - per-scan overrun detection
  - overrun event counter

- Added missing contract artifact:
  - `docs/timing-budget-v3.md`

- Updated docs index:
  - added `docs/timing-budget-v3.md` to active V3 docs.

## 2026-02-28 (Contract Artifact Completion + Conflict Cleanup)

### Completed

- Resolved merge conflict markers in:
  - `docs/api-contract-v3.md`
  - `docs/acceptance-matrix-v3.md`

- Added missing required V3 artifacts listed by source contract:
  - `docs/fault-policy-v3.md`
  - `docs/dependency-topology-rules.md`

- Updated `docs/INDEX.md` to include the two new artifacts in active V3 docs.

## 2026-02-28 (Phase 0 Contract Alignment: Snapshot Metrics)

### Completed

- Updated `docs/api-contract-v3.md` runtime snapshot contract:
  - Added explicit `metrics` object shape and required fields.
  - Added snapshot invariants:
    - `metrics.scanBudgetUs == scanIntervalMs * 1000`
    - `metrics.queueDepth <= metrics.queueCapacity`
  - Clarified HTTP snapshot response mirrors WebSocket runtime snapshot payload.

- Updated `docs/acceptance-matrix-v3.md`:
  - Added `AT-API-008` (metrics shape/presence)
  - Added `AT-API-009` (scan budget invariant)
  - Added `AT-API-010` (queue depth capacity invariant)

## 2026-02-28 (Phase 0 Hardware Baseline Capture)

### Test Inputs

- Idle capture file: `docs/snapshot-baseline.csv` (120 samples, ~2 minutes).
- Stress capture file: `docs/snapshot-stress.csv` (120 samples, ~2 minutes).
- Hardware: ESP32 DOIT DevKit (live `/api/snapshot` polling).

### Measured Results

- Idle:
  - `scan_avg_us`: `1373.92`
  - `scan_min_us`: `0`
  - `scan_max_us`: `4361`
  - `command_latency_last_avg_us`: `0`
  - `command_latency_max_peak_us`: `0`
  - `queue_depth_max`: `0`
  - `queue_hwm_max`: `0`
  - `overrun_end`: `0`

- Stress:
  - `scan_avg_us`: `1198.61`
  - `scan_min_us`: `0`
  - `scan_max_us`: `2710`
  - `command_latency_last_avg_us`: `739.95`
  - `command_latency_last_max_us`: `7076`
  - `command_latency_max_peak_us`: `7296`
  - `queue_depth_max`: `0`
  - `queue_hwm_max`: `1`
  - `overrun_end`: `0`

### Observations

- No scan overruns observed in either capture (`scanOverrunCount` remained `0`).
- Queue behavior remained stable under stress (`queueHighWaterMark` peaked at `1` of `16`).
- Command latency telemetry was exercised under stress and remained bounded in this run.

## 2026-02-28 (Phase 1 Skeleton Refactor: Interface-First Start)

### Completed

- Introduced initial layered source skeleton under `src/`:
  - `src/kernel/`
  - `src/runtime/`
  - `src/control/`
  - `src/storage/`
  - `src/portal/`
  - `src/platform/`

- Added interface headers for type extraction (no behavior changes):
  - `src/kernel/card_model.h` (card model enums + `LogicCard` DTO)
  - `src/control/command_dto.h` (run/input modes + kernel command DTO)
  - `src/runtime/shared_snapshot.h` (`SharedRuntimeSnapshotT<N>` DTO)

- Updated `src/main.cpp` to consume extracted interfaces:
  - removed in-file duplicate type definitions for `LogicCard`, `KernelCommand`, and shared snapshot struct.
  - retained existing runtime behavior and function logic.

### Notes

- This is a structure-only extraction step aligned with kickoff Phase 1 intent ("interfaces first, no behavior change").
- Local compile verification remains blocked in this shell because `platformio` / `pio` CLI is not available on PATH.

