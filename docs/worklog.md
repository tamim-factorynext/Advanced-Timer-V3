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

## 2026-02-28 (Phase 1 Skeleton Refactor: Enum/Parse Codec Extraction)

### Completed

- Extracted enum/string/parse helpers from `src/main.cpp` into kernel module:
  - `src/kernel/enum_codec.h`
  - `src/kernel/enum_codec.cpp`

- Updated `src/main.cpp`:
  - added `#include "kernel/enum_codec.h"`
  - removed in-file enum codec macros/helpers (`toString`, `tryParse*`, `parseOrDefault` block)
  - retained existing call sites and behavior.

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `enum_codec.h`.

### Verification

- Build passed using explicit PlatformIO binary path:
  - `C:\\Users\\Admin\\.platformio\\penv\\Scripts\\platformio.exe run`

## 2026-02-28 (Phase 1 Skeleton Refactor: Snapshot Serialization Declarations)

### Completed

- Added runtime declaration header:
  - `src/runtime/snapshot_json.h`
  - Declares:
    - `copySharedRuntimeSnapshot(...)`
    - `appendRuntimeSnapshotCard(...)`
    - `serializeRuntimeSnapshot(...)`

- Updated `src/main.cpp`:
  - Includes `runtime/snapshot_json.h`.
  - Switched snapshot alias to concrete struct form:
    - `struct SharedRuntimeSnapshot : SharedRuntimeSnapshotT<TOTAL_CARDS> {};`
  - No runtime behavior changes.

- Updated runtime layer inventory:
  - `src/runtime/README.md` now lists `snapshot_json.h`.

### Verification

- Build passed:
  - `C:\\Users\\Admin\\.platformio\\penv\\Scripts\\platformio.exe run`

## 2026-02-28 (Architecture Direction Lock-In: RTC + Initial Family Targets)

### Completed

- Captured architectural decisions in `docs/decisions.md`:
  - `DEC-0003`: RTC implementation baseline is `RTClib` using `RTCMillis` now, NTP time sync, and future DS3231 migration on same library.
  - `DEC-0004`: initial bring-up target is exactly 2 `RTC` scheduler cards and 2 `MATH` cards, with no extra empty placeholder slots in this phase.

- Updated dependency baseline:
  - Added `adafruit/RTClib` to `platformio.ini` `lib_deps`.

## 2026-02-28 (RTC Granularity Direction: Minute-Level Only)

### Completed

- Added decision:
  - `DEC-0005` in `docs/decisions.md`:
    - RTC schedule configuration is minute-granularity only.
    - `second` and millisecond-level schedule fields are unsupported and must be rejected.

- Updated contracts:
  - `requirements-v3-contract.md` Section 8.6:
    - removed required `second` schedule field.
    - added minute-level granularity rule.
  - `docs/schema-v3.md`:
    - removed `second` from RTC schedule schema.
    - added validation rule `V-CFG-022` to reject sub-minute schedule fields.

- Updated acceptance mapping:
  - `docs/acceptance-matrix-v3.md`:
    - added `AT-RTC-006` for rejecting `second`/millisecond RTC schedule fields.
    - shifted previous RTC TBD placeholder to `AT-RTC-007`.

## 2026-02-28 (Phase 1 Skeleton Refactor: Portal Route Declarations)

### Completed

- Added portal declaration surface header:
  - `src/portal/routes.h`
  - Includes HTTP route handler declarations and WebSocket route declarations only (no implementation move).

- Updated `src/main.cpp`:
  - includes `portal/routes.h`
  - removed duplicate portal-route forward declarations from local prototype block.
  - kept all route implementations in `main.cpp` unchanged.

- Updated portal layer inventory:
  - `src/portal/README.md` now lists `routes.h`.

### Verification

- Build passed:
  - `C:\\Users\\Admin\\.platformio\\penv\\Scripts\\platformio.exe run`

## 2026-02-28 (Phase 1 Skeleton Refactor: Storage Lifecycle Declarations)

### Completed

- Added storage declaration surface header:
  - `src/storage/config_lifecycle.h`
  - Includes declarations for config persistence/lifecycle helpers only (no implementation move).

- Updated `src/main.cpp`:
  - includes `storage/config_lifecycle.h`
  - removed duplicate storage/config-lifecycle forward declarations from local prototype block.
  - kept all implementations in `main.cpp` unchanged.

- Updated storage layer inventory:
  - `src/storage/README.md` now lists `config_lifecycle.h`.

### Verification

- Build passed:
  - `C:\\Users\\Admin\\.platformio\\penv\\Scripts\\platformio.exe run`

## 2026-02-28 (End-Of-Day Handoff)

### Current State

- Versioning baseline is active:
  - `V2` = frozen PoC baseline (`README.md`)
  - `V3` = active rewrite track (`requirements-v3-contract.md` + `docs/*-v3.md`)

- Phase 0 status:
  - Runtime metrics instrumentation added and exposed in `/api/snapshot`.
  - Timing budget contract added (`docs/timing-budget-v3.md`).
  - Baseline captures completed and logged:
    - `docs/snapshot-baseline.csv`
    - `docs/snapshot-stress.csv`

- Contract/doc integrity:
  - Required artifact set now exists (including fault policy and dependency topology).
  - Prior merge markers were cleaned from V3 docs.

- Architecture decisions locked:
  - `DEC-0003`: RTC stack is `RTClib` + `RTCMillis` now, DS3231 later, NTP sync.
  - `DEC-0004`: bring-up target includes exactly 2 RTC cards + 2 MATH cards.
  - `DEC-0005`: RTC schedule granularity is minute-level only (no second/ms config).

- Phase 1 refactor status (interfaces-first, no behavior move):
  - Added layered skeleton folders under `src/`.
  - Extracted interfaces:
    - `src/kernel/card_model.h`
    - `src/kernel/enum_codec.h/.cpp`
    - `src/control/command_dto.h`
    - `src/runtime/shared_snapshot.h`
    - `src/runtime/snapshot_json.h`
    - `src/portal/routes.h`
    - `src/storage/config_lifecycle.h`
  - Implementations remain in `src/main.cpp` intentionally.

- Build status:
  - Build is green after latest changes.
  - Current `platformio.ini` includes explicit SPI/Wire include paths to satisfy `RTClib` dependency resolution in this environment.

### Exact Start Plan For Next Session

1. Continue Phase 1 declaration extraction:
   - Add `src/control/runtime_control.h` declarations for command/control surface (`setRunMode`, breakpoint, force/mask, queue apply path).
   - Keep implementations in `main.cpp`.
2. Extract runtime snapshot implementation to `src/runtime/snapshot_json.cpp` (first implementation move), keep function signatures stable.
3. Begin RTC/MATH bring-up scaffolding:
   - define initial capacities and IDs for 2 RTC + 2 MATH cards (without placeholder empty slots).
   - add validation enforcement for minute-only RTC schedule fields (`V-CFG-022`) in firmware validation path.
4. Rebuild and capture one quick smoke snapshot to confirm no regression.

## 2026-03-01 (V3 Runtime Card Model Slice 1: MATH/RTC First-Class Allocation)

### Session Summary

Executed the first "future-first" migration slice by removing legacy-only runtime card allocation assumptions and promoting `MATH`/`RTC` to active runtime families.

### Completed

- Updated runtime family allocation in `src/main.cpp`:
  - Added fixed bring-up capacities: `NUM_MATH=2`, `NUM_RTC=2`.
  - Expanded `TOTAL_CARDS` to include `MATH` and `RTC`.
  - Added `MATH_START` and `RTC_START` boundaries.

- Updated default card initialization:
  - Added safe defaults for `MathCard` and `RtcCard`.
  - RTC scheduler channels now default to RTC family IDs (not DI IDs).

- Updated deterministic runtime dispatch:
  - Replaced legacy ID-range processing assumptions with `card.type` dispatch in `processCardById`.
  - Added explicit `processMathCard(...)` and `processRtcCard(...)` handlers.
  - Changed scan order to strict ascending `cardId` (`cursor % TOTAL_CARDS`) for deterministic contract alignment.

- Updated config validation semantics:
  - Enforced fixed family slot mapping by `cardId` (`DI|DO|AI|SIO|MATH|RTC`).
  - Added `Mode_None` acceptance for `MATH` and `RTC`.
  - Added operator allowance for `MATH`/`RTC` reference targets.
  - Added RTC set/reset rejection (`RTC set/reset is unsupported`).

- Updated decision log:
  - Added `DEC-0010` documenting this migration step.

### Evidence

- Build command:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
- Result:
  - `SUCCESS` (2026-03-01, local workspace build)

### Next Slice Candidates

1. Move config/API payload shape from legacy `config.cards[*].type/id` structure toward V3 canonical `cardType/cardId/config` semantics behind a strict schema gate.
2. Replace transitional MATH bridge logic with contract-grade typed MATH pipeline config and validators (`AT-MATH-*` alignment).
3. Introduce profile-backed family capacities/gates from one source of truth instead of hardcoded runtime constants.

## 2026-03-01 (V3 Config/API Slice 2: Envelope Normalization + Legacy Bridge)

### Session Summary

Implemented a contract-first normalization path for config lifecycle endpoints so staged save/validate/commit now operate on a normalized V3 envelope and card payload, with an explicit transitional bridge for legacy card shape.

### Completed

- Added V3 config/API constants in firmware:
  - `apiVersion=2.0`
  - `schemaVersion=2.0.0`

- Added unified normalization flow in `src/main.cpp`:
  - `normalizeConfigRequest(...)`
  - V3 card mapper (`cardId/cardType/config` -> legacy internal card model)
  - legacy cards bridge detection and normalization (`usedLegacyCardsBridge`)
  - unsupported version checks:
    - `UNSUPPORTED_API_VERSION`
    - `UNSUPPORTED_SCHEMA_VERSION`

- Updated HTTP config endpoints to use normalization:
  - `/api/config/staged/save`
  - `/api/config/staged/validate`
  - `/api/config/commit`

- Updated response shape for these endpoints:
  - `apiVersion`
  - `requestId`
  - `status` (`SUCCESS|FAILURE`)
  - `errorCode`
  - `message`
  - endpoint-specific payload fields (`stagedVersion`, `validation`, `activeVersion`, etc.)

- Added bridge observability:
  - persisted `bridge.usedLegacyCardsBridge`
  - response field `usedLegacyCardsBridge`
  - serial log marker when legacy bridge path is used

### Evidence

- Build command:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
- Result:
  - `SUCCESS` (2026-03-01, local workspace build)

### Known Transitional Gaps

1. Mapper currently implements a deterministic subset bridge for V3 typed cards into existing runtime fields; full typed model persistence is pending.
2. `GET /api/config/active` still returns internal legacy card objects (with V3 response envelope metadata) until full schema model separation lands.

## 2026-03-01 (V3 Persistence Slice 3: Native Storage Envelope)

### Session Summary

Completed the next migration slice by switching config persistence from legacy card arrays to native V3 envelope objects, while keeping safe legacy read fallback.

### Completed

- Added native V3 serialization helpers in `src/main.cpp`:
  - `serializeCardsToV3Array(...)`
  - `buildV3ConfigEnvelope(...)`
  - condition translation helpers for set/reset blocks.

- Switched persistence writers to V3 envelope:
  - `saveCardsToPath(...)` now writes V3 object form.
  - `saveLogicCardsToLittleFS()` now routes through `saveCardsToPath(...)`.
  - staged save endpoint now writes V3 envelope to `/config_staged.json`.

- Switched readers to dual-format load:
  - `loadCardsFromPath(...)` accepts:
    - native V3 object via normalization,
    - legacy array fallback (migration safety).
  - `loadLogicCardsFromLittleFS()` now routes through `loadCardsFromPath(...)`.

- Updated active config API export:
  - `GET /api/config/active` now emits V3 envelope-style `config`.

### Evidence

- Build command:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
- Result:
  - `SUCCESS` (2026-03-01, local workspace build)

### Remaining Transitional Gaps

1. V3 <-> internal model mapping is deterministic but still bridge-based; full typed runtime model separation remains pending.
2. Some operator/state mappings are represented via bridge-safe equivalents and should be hardened by acceptance tests before bridge removal.

## 2026-03-01 (V3 Model Slice 4: Typed Per-Family Struct Foundation)

### Session Summary

Started the structural split away from unified legacy `LogicCard` by introducing typed per-family V3 config structs and a reusable bridge layer.

### Completed

- Added new kernel typed model files:
  - `src/kernel/v3_card_types.h`
  - `src/kernel/v3_card_bridge.h`
  - `src/kernel/v3_card_bridge.cpp`

- Added typed family definitions:
  - `V3DiConfig`, `V3DoConfig`, `V3AiConfig`, `V3SioConfig`, `V3MathConfig`, `V3RtcConfig`
  - Shared wrapper: `V3CardConfig`

- Added migration bridge functions:
  - `legacyToV3CardConfig(...)`
  - `v3CardConfigToLegacy(...)`

- Wired bridge into active serialization path:
  - `serializeCardsToV3Array(...)` now uses typed bridge output as source for V3 payload emission.

- Updated kernel layer inventory:
  - `src/kernel/README.md` includes new typed model interfaces.

### Evidence

- Build command:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
- Result:
  - `SUCCESS` (2026-03-01, local workspace build)

### Remaining Transitional Gaps

1. Runtime execution path still evaluates `LogicCard`; typed structs currently own schema/bridge side, not full kernel processing.
2. Full typed-validator and typed-commit internals are next required steps before removing `LogicCard` as primary runtime DTO.

## 2026-03-01 (V3 Validation Slice 5: Family-Aware Condition Field Rules)

### Session Summary

Applied strict family-aware validation for V3 condition clauses so set/reset references only use runtime fields that are meaningful for the referenced source card family.

### Completed

- Added V3 signal capability model:
  - `V3SignalSupport`
  - `signalSupportForFamily(...)`
  - file: `src/kernel/v3_card_types.h`

- Tightened V3 clause mapping in `src/main.cpp`:
  - validates source `cardType` map up-front by `cardId` slot.
  - validates source field applicability by family.
  - enforces bool-like field operators (`logicalState`, `physicalState`, `triggerFlag`) as `EQ|NEQ` only.
  - enforces `missionState` mapping and rejects invalid source families/operators.
  - rejects out-of-range clause source `cardId`.

- Updated V3->legacy apply path to consume these validations before conversion.

### Evidence

- Build command:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
- Result:
  - `SUCCESS` (2026-03-01, local workspace build)

### Remaining Transitional Gaps

1. Runtime still executes on transitional `LogicCard`; typed runtime-state engine migration is still pending.
2. Acceptance tests for invalid clause-source combinations should be added next to lock this behavior.

## 2026-03-01 (V3 Acceptance Slice 6: Executable Condition-Rule Tests)

### Session Summary

Added executable native tests for per-family condition source field/operator constraints and extracted those rules into a reusable kernel module.

### Completed

- Added kernel rule module:
  - `src/kernel/v3_condition_rules.h`
  - `src/kernel/v3_condition_rules.cpp`
  - Functions:
    - `parseV3CardTypeToken(...)`
    - `isV3FieldAllowedForSourceType(...)`
    - `isV3OperatorAllowedForField(...)`

- Wired runtime normalization path to this module:
  - `src/main.cpp` now uses the shared rule helpers in V3 clause mapping.

- Added native Unity acceptance tests:
  - `test/test_v3_condition_rules/test_main.cpp`
  - Test coverage includes:
    - reject `AI.logicalState`
    - reject `RTC.missionState`
    - allow `DO/SIO.missionState` only with `EQ`
    - bool-field operator restrictions
    - card-type token parsing

- Added PlatformIO native test env:
  - `platformio.ini` `[env:native]`
  - Restored default firmware build target via:
    - `[platformio] default_envs = esp32doit-devkit-v1`

### Evidence

- Native acceptance tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (5/5)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Validation Slice 7: Typed Parse Before Legacy Bridge)

### Session Summary

Refactored normalization so V3 payloads are parsed and validated into typed `V3CardConfig` first, then converted to runtime `LogicCard` via bridge.

### Completed

- Added typed parse stage in `src/main.cpp`:
  - `parseV3CardToTyped(...)`
  - Parses card-family config into `V3CardConfig` with family-specific checks.

- Updated normalization conversion flow:
  - `buildLegacyCardsFromV3Cards(...)` now:
    - builds source type map,
    - parses each V3 card into typed model,
    - converts typed model to `LogicCard` using `v3CardConfigToLegacy(...)`.

- Preserved RTC schedule handoff behavior:
  - RTC typed config fields are applied to scheduler channel state during bridge conversion phase.

### Evidence

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (5/5)

## 2026-03-01 (V3 Acceptance Slice 8: Payload-Level Fixture Tests For Parse Boundary)

### Session Summary

Added payload-level JSON fixture validation coverage for clause-source semantics and integrated it into normalize/parse flow before typed-card parsing.

### Completed

- Added payload validator module:
  - `src/kernel/v3_payload_rules.h`
  - `src/kernel/v3_payload_rules.cpp`
  - Main entry:
    - `validateV3PayloadConditionSources(...)`

- Integrated payload validator into parse boundary:
  - `buildLegacyCardsFromV3Cards(...)` now calls payload validator before typed parse/bridge conversion.

- Added fixture-based native tests:
  - `test/test_v3_payload_parse/test_main.cpp`
  - Cases:
    - reject `AI.logicalState`
    - reject `RTC.missionState`
    - reject `missionState` with `NEQ`
    - accept `DO.missionState` with `EQ`

- Updated native env deps/build filters:
  - `platformio.ini` (`ArduinoJson` available in native tests, payload rules module included).

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (9/9 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Architecture Slice 9: Storage-Owned Normalize Pipeline)

### Session Summary

Moved V3 config normalization/bridge orchestration out of `main.cpp` into a dedicated storage module, while keeping endpoint behavior stable.

### Completed

- Added storage normalization module:
  - `src/storage/v3_normalizer.h`
  - `src/storage/v3_normalizer.cpp`
  - Entry function:
    - `normalizeConfigRequestWithLayout(...)`

- Updated main normalization boundary:
  - `normalizeConfigRequest(...)` in `src/main.cpp` now:
    - builds profile context (`TOTAL_CARDS`, family start offsets),
    - supplies baseline cards,
    - calls storage normalizer,
    - applies RTC schedule outputs,
    - performs final legacy-array semantic validation.

- Updated storage layer inventory:
  - `src/storage/README.md` now lists `v3_normalizer.h`.

### Evidence

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (13/13)

## 2026-03-01 (V3 Runtime Slice 10: DI Runtime Module Activation + Acceptance)

### Session Summary

Activated the extracted DI runtime module in the live scan path and added deterministic native acceptance tests for DI runtime-state transitions.

### Completed

- Resolved typed-model naming collision between config model and runtime module:
  - renamed runtime config type to `V3DiRuntimeConfig`.
  - files:
    - `src/kernel/v3_di_runtime.h`
    - `src/kernel/v3_di_runtime.cpp`
    - `src/main.cpp` (`processDICard(...)` runtime call site)

- Kept runtime ownership explicit:
  - `processDICard(...)` now executes DI behavior through `runV3DiStep(...)` using a runtime-only DTO (`V3DiRuntimeConfig` + `V3DiRuntimeState`).

- Added DI runtime native acceptance tests:
  - `test/test_v3_di_runtime/test_main.cpp`
  - coverage includes:
    - reset-condition inhibition and runtime reset semantics
    - set-condition false idle semantics
    - rising-edge qualification path
    - non-matching edge rejection
    - debounce-window filtering behavior

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (18/18 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 11: DO Runtime Module Activation + Acceptance)

### Session Summary

Extracted DO runtime transition logic into a dedicated kernel module and switched `processDOCard(...)` to a runtime DTO + step-function path, with deterministic native acceptance coverage.

### Completed

- Added DO runtime kernel module:
  - `src/kernel/v3_do_runtime.h`
  - `src/kernel/v3_do_runtime.cpp`
  - entrypoint:
    - `runV3DoStep(...)`

- Updated runtime integration in `src/main.cpp`:
  - `processDOCard(...)` now maps `LogicCard` to `V3DoRuntimeState`, executes `runV3DoStep(...)`, and maps results back.
  - hardware write ownership remains in `main.cpp` (`driveDOHardware(...)`).

- Removed obsolete transitional state:
  - deleted `gPrevSetCondition` usage/state reset path (no behavioral impact; retrigger semantics are level-based while idle/finished).

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_do_runtime.h`.

- Added native DO runtime acceptance tests:
  - `test/test_v3_do_runtime/test_main.cpp`
  - coverage includes:
    - reset-condition precedence and idle reset
    - immediate-mode activation
    - normal-mode `OnDelay -> Active` transition
    - repeat-limit completion to `Finished`
    - gated-mode drop to `Idle` when gate deasserts

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (23/23 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

