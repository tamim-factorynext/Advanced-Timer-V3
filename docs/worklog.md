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

## 2026-03-01 (V3 Runtime Slice 12: SIO Runtime Boundary + Acceptance)

### Session Summary

Established explicit SIO runtime ownership by introducing a dedicated SIO runtime boundary module and wiring `processSIOCard(...)` directly to runtime step execution.

### Completed

- Added SIO runtime boundary module:
  - `src/kernel/v3_sio_runtime.h`
  - `src/kernel/v3_sio_runtime.cpp`
  - entrypoint:
    - `runV3SioStep(...)`

- Updated runtime integration:
  - `src/main.cpp` now executes SIO state transitions through `runV3SioStep(...)` instead of delegating to `processDOCard(...)`.
  - SIO keeps service/runtime semantics explicit while sharing DO state machine implementation via runtime wrapper internals.

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_sio_runtime.h`.

- Added native SIO runtime acceptance tests:
  - `test/test_v3_sio_runtime/test_main.cpp`
  - coverage includes:
    - reset-condition precedence to `Idle`
    - immediate activation to running mission state
    - normal-mode completion to `Finished`
    - gated-mode drop to `Idle` when gate deasserts

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (27/27 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 13: MATH Runtime Module Activation + Acceptance)

### Session Summary

Extracted transitional MATH evaluation behavior into a dedicated runtime module and switched `processMathCard(...)` to execute via typed runtime DTO + step API.

### Completed

- Added MATH runtime module:
  - `src/kernel/v3_math_runtime.h`
  - `src/kernel/v3_math_runtime.cpp`
  - entrypoint:
    - `runV3MathStep(...)`

- Updated runtime integration:
  - `src/main.cpp` `processMathCard(...)` now maps legacy card fields into `V3MathRuntimeConfig`/`V3MathRuntimeState`, executes `runV3MathStep(...)`, and maps results back.
  - Preserved existing transitional deterministic semantics:
    - reset uses fallback value (`setting3`)
    - set=false clears flags without recompute
    - set=true computes `inputA + inputB` with uint32 saturation
    - optional clamp window applies when `startOffMs >= startOnMs`

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_math_runtime.h`.

- Added native MATH runtime acceptance tests:
  - `test/test_v3_math_runtime/test_main.cpp`
  - coverage includes:
    - reset fallback behavior
    - set=false behavior
    - sum behavior
    - clamp behavior
    - uint32 saturation behavior

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (32/32 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 14: RTC Runtime Module + Minute Semantics Tests)

### Session Summary

Extracted RTC runtime-state handling into a dedicated module, wired minute-scheduler matching through shared RTC runtime helpers, and added native acceptance tests for trigger-window and minute-level matching semantics.

### Completed

- Added RTC runtime module:
  - `src/kernel/v3_rtc_runtime.h`
  - `src/kernel/v3_rtc_runtime.cpp`
  - runtime entrypoints/helpers:
    - `runV3RtcStep(...)`
    - `v3RtcChannelMatchesMinute(...)`
    - `v3RtcMinuteKey(...)`

- Updated runtime integration in `src/main.cpp`:
  - `processRtcCard(...)` now executes via `runV3RtcStep(...)`.
  - `setRtcCardStateCommand(...)` records trigger start timestamp on assert.
  - minute scheduler matching now uses shared RTC runtime minute helpers.
  - preserved scheduler ownership on service core while making minute matching logic reusable/testable.

- Added native RTC runtime acceptance tests:
  - `test/test_v3_rtc_runtime/test_main.cpp`
  - coverage includes:
    - logical-false trigger clear behavior
    - trigger window hold/expiry behavior
    - minute wildcard matching behavior
    - minute key progression behavior

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_rtc_runtime.h`.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (37/37 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 15: AI Runtime Module + Typed Field Semantics)

### Session Summary

Covered AI before operator/state cleanup by extracting AI processing into a dedicated runtime module with card-specific field semantics (`inputMin/inputMax/outputMin/outputMax/emaAlpha`) and wiring `processAICard(...)` through it.

### Completed

- Added AI runtime module:
  - `src/kernel/v3_ai_runtime.h`
  - `src/kernel/v3_ai_runtime.cpp`
  - entrypoint:
    - `runV3AiStep(...)`

- Updated runtime integration:
  - `src/main.cpp` `processAICard(...)` now maps legacy fields to `V3AiRuntimeConfig` and executes `runV3AiStep(...)`.
  - Removed local AI-only math helpers from `main.cpp` by moving mapping/filtering logic into kernel runtime module.

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_ai_runtime.h`.

- Added native AI runtime acceptance tests:
  - `test/test_v3_ai_runtime/test_main.cpp`
  - coverage includes:
    - alpha `1000` direct tracking
    - alpha `0` hold behavior
    - input bounds order-independence
    - equal-range mapping behavior
    - alpha clamp at upper bound

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (42/42 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 16: Family-Aware Mission Status Cleanup)

### Session Summary

Completed operator/status cleanup after AI extraction by replacing DO-only mission-state assumptions with explicit family-aware status helpers for `DO/SIO`.

### Completed

- Added status runtime helper module:
  - `src/kernel/v3_status_runtime.h`
  - `src/kernel/v3_status_runtime.cpp`
  - helpers:
    - `isMissionRunning(...)`
    - `isMissionFinished(...)`
    - `isMissionStopped(...)`

- Updated operator evaluation in `src/main.cpp`:
  - `Op_Running`, `Op_Finished`, `Op_Stopped` now route through family-aware helper functions instead of direct DO-state checks.

- Added native status acceptance tests:
  - `test/test_v3_status_runtime/test_main.cpp`
  - coverage includes:
    - DO mission-state truth table
    - SIO mission-state truth table
    - non-mission families rejected for mission-state operators

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_status_runtime.h`.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (45/45 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 17: LogicCard Access Isolation via Runtime Adapters)

### Session Summary

Started explicit step-by-step phase-out of direct `LogicCard` usage by isolating legacy-field mapping to one adapter module and updating all runtime callsites to consume typed family DTOs through that boundary.

### Completed

- Added runtime adapter boundary module:
  - `src/kernel/v3_runtime_adapters.h`
  - `src/kernel/v3_runtime_adapters.cpp`
  - provides family-specific mapping helpers for:
    - `DI`, `AI`, `DO`, `SIO`, `MATH`, `RTC`

- Updated runtime callsites in `src/main.cpp`:
  - `processDICard(...)`, `processAICard(...)`, `processDOCard(...)`,
    `processSIOCard(...)`, `processMathCard(...)`, and `processRtcCard(...)`
    now use adapter helpers instead of manual per-field mapping blocks.
  - removed remaining local helper duplication tied to direct mapping logic.

- Added adapter acceptance tests:
  - `test/test_v3_runtime_adapters/test_main.cpp`
  - coverage includes:
    - AI config mapping
    - DO state roundtrip mapping
    - RTC trigger-duration/start mapping

- Updated kernel layer inventory:
  - `src/kernel/README.md` now lists `v3_runtime_adapters.h`.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (48/48 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 18: Typed Runtime State Store Ownership)

### Session Summary

Moved per-family mutable runtime state ownership out of `LogicCard` into a typed runtime store (`DI/DO/AI/SIO/MATH/RTC`), while keeping a controlled mirror-back bridge for legacy snapshot/API compatibility.

### Completed

- Added typed runtime store module:
  - `src/kernel/v3_runtime_store.h`
  - `src/kernel/v3_runtime_store.cpp`
  - capabilities:
    - family/index-safe runtime state lookup
    - runtime store sync from legacy cards during config lifecycle transitions
    - legacy mirror function for time-bounded compatibility

- Updated runtime ownership flow in `src/main.cpp`:
  - added family-typed runtime arrays as state owners:
    - `gDiRuntime`, `gDoRuntime`, `gAiRuntime`, `gSioRuntime`, `gMathRuntime`, `gRtcRuntime`
  - added `syncRuntimeStateFromCards()` and invoked it at deterministic transition points:
    - safe defaults initialization
    - config load
    - active config apply
  - refactored process paths to use typed runtime store as mutation target:
    - `processDICard(...)`
    - `processAICard(...)`
    - `processDOCard(...)`
    - `processSIOCard(...)`
    - `processMathCard(...)`
    - `processRtcCard(...)`
  - `setRtcCardStateCommand(...)` now updates RTC typed runtime state first, then mirrors back.

- Added native acceptance tests for runtime store:
  - `test/test_v3_runtime_store/test_main.cpp`
  - coverage includes:
    - sync from card array into family stores
    - mirror from family store back into legacy card state fields
    - lookup rejection on wrong family or out-of-range index

- Updated kernel inventory:
  - `src/kernel/README.md` now lists `v3_runtime_store.h`.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (54/54 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 19: Config Sanitization for Runtime-Only Fields)

### Session Summary

Stopped accepting runtime-mutable state from config deserialization so persisted/staged config can no longer rehydrate transient execution state into runtime ownership.

### Completed

- Added config sanitization module:
  - `src/kernel/v3_config_sanitize.h`
  - `src/kernel/v3_config_sanitize.cpp`
  - APIs:
    - `sanitizeConfigCardRuntimeFields(...)`
    - `sanitizeConfigCardsRuntimeFields(...)`

- Wired sanitization into config deserialization:
  - `src/main.cpp`
  - `deserializeCardsFromArray(...)` now sanitizes all cards after parsing.

- Runtime-only fields now reset from config payloads:
  - `logicalState`, `physicalState`, `triggerFlag`, `currentValue`, `repeatCounter`
  - runtime timers/state per family (with family-safe preservation of true config fields like AI/MATH ranges and RTC trigger duration setting).

- Added native acceptance tests:
  - `test/test_v3_config_sanitize/test_main.cpp`
  - coverage includes:
    - DO runtime field clearing
    - AI config range preservation with runtime reset
    - RTC runtime trigger-start clearing while preserving config duration
    - array-wide sanitize behavior

- Updated kernel inventory:
  - `src/kernel/README.md` now lists `v3_config_sanitize.h`.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (58/58 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 20: Legacy Config Contract Drops Runtime Fields)

### Session Summary

Removed runtime-only fields from the legacy card config contract and made validation reject them explicitly, while preserving runtime observability through snapshot payloads.

### Completed

- Tightened legacy config serializer/deserializer in `src/main.cpp`:
  - `serializeCardToJson(...)` no longer emits runtime-only fields:
    - `logicalState`, `physicalState`, `triggerFlag`, `currentValue`, `repeatCounter`, `state`
  - `deserializeCardFromJson(...)` no longer reads runtime-only fields.
  - `startOnMs/startOffMs` are now emitted/read only for card families where these are true config fields (`AI`, `MATH`).

- Tightened config validation in `src/main.cpp`:
  - `validateConfigCardsArray(...)` now rejects runtime-only fields in config payloads.
  - `startOnMs/startOffMs` are accepted only for `AI/MATH`; rejected for other families.

- Runtime snapshot contract remains unchanged:
  - runtime observability still comes from snapshot serialization (`appendRuntimeSnapshotCard(...)`).

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (58/58 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 21: External Legacy Payload Bridge Removal)

### Session Summary

Removed the remaining external legacy `config.cards` payload acceptance path so HTTP config APIs now require V3 typed card payload shape (`cardId/cardType/config`) only.

### Completed

- Updated shared normalizer:
  - `src/storage/v3_normalizer.cpp`
  - `normalizeConfigRequestWithLayout(...)` now rejects legacy-shaped cards payloads with:
    - `INVALID_REQUEST`
    - reason: legacy payload is no longer supported.

- Aligned normalizer output card serialization to strict config contract:
  - removed runtime-only fields from normalizer legacy card JSON projection.
  - `startOnMs/startOffMs` emitted only for `AI/MATH`.

- Removed stale in-file legacy conversion helpers from `src/main.cpp`:
  - deleted dead local `buildLegacyCardsFromV3Cards(...)` and shape-detection helpers.
  - simplified `normalizeConfigRequest(...)` RTC schedule propagation path (no legacy branch logging).

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (58/58 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 22: Bridge Metadata and Flag Removal)

### Session Summary

Removed leftover legacy-bridge flags/metadata from normalizer signatures and API responses so config flow is now V3-only without compatibility toggles.

### Completed

- Removed `usedLegacyBridge` from config normalization interfaces:
  - `src/storage/v3_normalizer.h`
  - `src/storage/v3_normalizer.cpp`
  - `src/main.cpp`

- Removed bridge metadata emission:
  - dropped `bridge.usedLegacyCardsBridge` and `bridge.bridgeVersion` in normalizer output.
  - dropped `usedLegacyCardsBridge` from staged/validate/commit API extras.

- Simplified `main.cpp` config handlers:
  - `handleHttpStagedSaveConfig(...)`
  - `handleHttpStagedValidateConfig(...)`
  - `handleHttpCommitConfig(...)`
  - `loadCardsFromPath(...)`
  - `normalizeConfigRequest(...)`

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (58/58 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 23: Typed Snapshot Ownership)

### Session Summary

Removed `LogicCard` from shared runtime snapshot ownership by introducing a dedicated runtime snapshot card DTO populated from typed runtime stores.

### Completed

- Added runtime snapshot card DTO:
  - `src/runtime/runtime_snapshot_card.h`

- Added snapshot card builder module:
  - `src/runtime/snapshot_card_builder.h`
  - `src/runtime/snapshot_card_builder.cpp`
  - builds per-card snapshot state from typed family runtime stores (`DI/DO/AI/SIO/MATH/RTC`).

- Updated shared snapshot structure:
  - `src/runtime/shared_snapshot.h`
  - `SharedRuntimeSnapshotT::cards` now uses `RuntimeSnapshotCard[N]` instead of `LogicCard[N]`.

- Updated snapshot population path in `src/main.cpp`:
  - `updateSharedRuntimeSnapshot(...)` now uses `buildRuntimeSnapshotCards(...)` instead of memcpy from `logicCards`.
  - `appendRuntimeSnapshotCard(...)` now consumes `RuntimeSnapshotCard`.

- Added native acceptance tests:
  - `test/test_v3_snapshot_card_builder/test_main.cpp`
  - coverage includes:
    - DO snapshot mapping from typed runtime state
    - mixed AI/RTC snapshot card generation

- Updated runtime layer inventory:
  - `src/runtime/README.md` now lists:
    - `runtime_snapshot_card.h`
    - `snapshot_card_builder.h`

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (60/60 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 24: Snapshot Publication Decoupled From Live LogicCard Reads)

### Session Summary

Completed the next decoupling step by removing live `LogicCard` reads from snapshot publication. Snapshot cards are now built from runtime metadata + typed runtime store, not from `logicCards` directly.

### Completed

- Added runtime card metadata module:
  - `src/runtime/runtime_card_meta.h`
  - `src/runtime/runtime_card_meta.cpp`
  - provides:
    - `RuntimeCardMeta`
    - `refreshRuntimeCardMetaFromCards(...)`

- Extended runtime store with index-based accessors:
  - `src/kernel/v3_runtime_store.h`
  - `src/kernel/v3_runtime_store.cpp`
  - added `runtime*StateAt(index, store)` helpers for all families.

- Refactored snapshot card builder to metadata-based input:
  - `src/runtime/snapshot_card_builder.h`
  - `src/runtime/snapshot_card_builder.cpp`
  - now consumes `RuntimeCardMeta` instead of `LogicCard`.

- Updated runtime snapshot path in `src/main.cpp`:
  - added `gRuntimeCardMeta[TOTAL_CARDS]`.
  - metadata is refreshed on config/runtime sync transitions via `syncRuntimeStateFromCards()`.
  - `updateSharedRuntimeSnapshot(...)` now calls:
    - `buildRuntimeSnapshotCards(gRuntimeCardMeta, TOTAL_CARDS, gRuntimeStore, ...)`

- Added native acceptance tests:
  - `test/test_v3_runtime_card_meta/test_main.cpp`
  - updated `test/test_v3_snapshot_card_builder/test_main.cpp` for metadata-based builder input.

- Updated runtime inventory:
  - `src/runtime/README.md` now lists `runtime_card_meta.h`.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (62/62 total across native suites)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 25: Runtime Signals Decoupled From LogicCard)

### Session Summary

Removed direct `LogicCard` dependency from runtime signal publication/evaluation inputs. Runtime signals now derive from typed runtime store plus per-card metadata (`RuntimeCardMeta`).

### Completed

- Refactored signal APIs to runtime-owned inputs:
  - `src/kernel/v3_runtime_signals.h`
  - `src/kernel/v3_runtime_signals.cpp`
  - changes:
    - `makeRuntimeSignal(...)` now consumes `RuntimeCardMeta + V3RuntimeStoreView`.
    - `refreshRuntimeSignalsFromRuntime(...)` added for full-array refresh.
    - `refreshRuntimeSignalAt(...)` now updates one signal from metadata + runtime store.

- Updated all runtime callsites in `src/main.cpp`:
  - initialization/config-load/apply-config now refresh signals via:
    - `refreshRuntimeSignalsFromRuntime(gRuntimeCardMeta, gRuntimeStore, ...)`
  - per-card runtime updates now use:
    - `refreshRuntimeSignalAt(gRuntimeCardMeta, gRuntimeStore, ..., cardId)`

- Updated native unit tests:
  - `test/test_v3_runtime_signals/test_main.cpp`
  - tests now validate signal generation from metadata + typed runtime states (no `LogicCard` inputs).

### Evidence

- Verification command availability in this shell:
  - `platformio`, `pio`, and `python -m platformio` are not installed/resolvable.
  - Acceptance commands to run in your PlatformIO environment:
    - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
    - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 26: Typed Config Ownership Through Commit Path)

### Session Summary

Moved config normalization/commit flow one step closer to V3 ownership by carrying typed `V3CardConfig[]` through normalization and using typed-to-legacy bridge only at final runtime apply/persistence boundaries.

### Completed

- Extended normalizer output contract:
  - `src/storage/v3_normalizer.h`
  - `src/storage/v3_normalizer.cpp`
  - `normalizeConfigRequestWithLayout(...)` now returns merged typed cards via `V3CardConfig* typedOut`.
  - typed output is derived from normalized merged cards (including RTC schedule fields).

- Updated main config pipeline:
  - `src/main.cpp`
  - `normalizeConfigRequest(...)` now accepts typed output buffer.
  - Added `buildLegacyCardsFromTyped(...)` bridge helper.
  - `handleHttpStagedSaveConfig(...)` now uses typed->legacy bridge instead of deserializing legacy JSON cards.
  - `handleHttpCommitConfig(...)` now commits from typed cards (`commitCards(const V3CardConfig*)`).
  - `loadCardsFromPath(...)` now builds runtime cards from typed normalized output.
  - Kept `commitLegacyCards(JsonArrayConst, ...)` as explicit transitional bridge for legacy restore snapshots.

### Migration Impact

- Reduced active config path dependence on legacy JSON-card reparse (`deserializeCardsFromArray`) after V3 normalization.
- Preserved deterministic behavior and fallback compatibility for legacy restore source while keeping boundary explicit.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 27: Typed-Only Normalize Contract in Main Pipeline)

### Session Summary

Removed legacy normalized-card array exposure from `main.cpp` normalize path. `normalizeConfigRequest(...)` now returns only typed cards (`V3CardConfig[]`) plus error metadata.

### Completed

- Refactored `normalizeConfigRequest(...)` signature in `src/main.cpp`:
  - before: returned `JsonDocument` + `JsonArrayConst` + typed cards.
  - now: returns typed cards only (`V3CardConfig* outTypedCards`) with `reason/errorCode`.

- Updated callsites to typed-only normalize flow:
  - `handleHttpStagedSaveConfig(...)`
  - `handleHttpStagedValidateConfig(...)`
  - `handleHttpCommitConfig(...)`
  - `loadCardsFromPath(...)`
  - all now call:
    - `normalizeConfigRequest(root, reason, errorCode, typedCards)`

- Kept existing legacy validation semantics internally:
  - `normalizeConfigRequest(...)` still runs `validateConfigCardsArray(...)` against a local normalized cards view, but does not expose that legacy shape to callsites.

### Migration Impact

- Main config pipeline now treats typed cards as the sole normalization output contract.
- Legacy card array format is further confined to transitional/internal boundaries.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 28: Normalize Uses Typed Validator, Not Legacy Array Validator)

### Session Summary

Completed the next decoupling step in normalization by removing its dependency on `validateConfigCardsArray(...)` and validating only typed cards in the V3 path.

### Completed

- Added typed config validator in `src/main.cpp`:
  - `validateTypedCardConfigs(const V3CardConfig* cards, uint8_t count, String& reason)`
  - validates:
    - fixed `cardId` ordering and family-slot alignment
    - per-family mode constraints (DI/DO/SIO)
    - AI ranges and EMA bounds
    - RTC schedule bounds (month/day/weekday/hour/minute)
    - condition source IDs and operator compatibility by source family

- Updated `normalizeConfigRequest(...)` to use typed validation:
  - replaced `validateConfigCardsArray(normalizedCards, reason)` with
    - `validateTypedCardConfigs(outTypedCards, TOTAL_CARDS, reason)`
  - keeps legacy validator out of normalize pipeline.

### Migration Impact

- V3 normalize path no longer depends on legacy JSON-card validator semantics.
- Legacy card-array validator remains only for explicit legacy/transitional paths.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 29: Extract Typed Validator to Kernel Module + Unit Tests)

### Session Summary

Moved typed config validation out of `main.cpp` into a reusable kernel module and added dedicated native tests for it.

### Completed

- Added new kernel validation module:
  - `src/kernel/v3_typed_config_rules.h`
  - `src/kernel/v3_typed_config_rules.cpp`
  - exposes:
    - `validateTypedCardConfigs(const V3CardConfig* cards, uint8_t count, uint8_t doStart, uint8_t aiStart, uint8_t sioStart, uint8_t mathStart, uint8_t rtcStart, std::string& reason)`

- Refactored `src/main.cpp`:
  - added include for new module.
  - removed in-file typed validator implementation.
  - `normalizeConfigRequest(...)` now calls kernel validator with layout boundaries.

- Added new native test suite:
  - `test/test_v3_typed_config_rules/test_main.cpp`
  - coverage includes:
    - valid typed layout acceptance
    - family-slot mismatch rejection
    - mode mismatch rejection (DO)
    - operator-source-family mismatch rejection
    - RTC minute bounds rejection

- Updated kernel inventory docs:
  - `src/kernel/README.md` now lists `v3_typed_config_rules.h`.

### Migration Impact

- Typed validation logic is now reusable, isolated, and testable without `main.cpp` coupling.
- Keeps migration aligned with contract-first kernel ownership boundaries.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 30: Extract Typed Card Parser to Kernel Module + Unit Tests)

### Session Summary

Moved V3 typed card parsing out of `main.cpp` and into a dedicated kernel module, then rewired normalization to consume the module directly.

### Completed

- Added parser module:
  - `src/kernel/v3_typed_card_parser.h`
  - `src/kernel/v3_typed_card_parser.cpp`
  - exposes:
    - `parseV3CardToTyped(...)` with explicit layout boundaries (`totalCards`, `doStart`, `aiStart`, `sioStart`, `mathStart`, `rtcStart`)
  - migrated helper logic from `main.cpp`:
    - mode mapping (`mapV3ModeToLegacy`)
    - clause/operator/threshold mapping
    - condition block mapping
    - family mapping for typed output

- Rewired normalizer:
  - `src/storage/v3_normalizer.cpp`
  - removed `extern` linkage to `main.cpp` parser.
  - now calls `parseV3CardToTyped(...)` from kernel parser module.

- Cleaned `main.cpp`:
  - removed in-file typed parse helper block and parser implementation.
  - kept only config pipeline orchestration and typed-bridge ownership boundaries.

- Added parser unit tests:
  - `test/test_v3_typed_card_parser/test_main.cpp`
  - coverage includes:
    - DI parse success
    - family slot mismatch rejection
    - invalid mission-state threshold rejection
    - RTC parse success

- Updated kernel inventory docs:
  - `src/kernel/README.md` now lists `v3_typed_card_parser.h`.

### Migration Impact

- Removes another large legacy-oriented parser block from `main.cpp`.
- Clarifies kernel ownership of typed parsing + typed validation as reusable modules.
- Keeps normalize path contract-first and module-driven.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 31: Move Normalize Orchestration Out of main.cpp)

### Session Summary

Completed extraction of normalize orchestration from `main.cpp` into a dedicated storage service module, leaving `main.cpp` as callsite orchestration only.

### Completed

- Added config service module:
  - `src/storage/v3_config_service.h`
  - `src/storage/v3_config_service.cpp`
  - exposes:
    - `normalizeV3ConfigRequestTyped(...)`
  - responsibilities:
    - calls `normalizeConfigRequestWithLayout(...)`
    - runs typed validator (`validateTypedCardConfigs(...)`)
    - returns typed cards + RTC schedule output for caller apply

- Rewired `main.cpp` callsites (`staged save`, `staged validate`, `commit`, `load`) to use storage service API.
- Removed old `normalizeConfigRequest(...)` implementation from `main.cpp`.
- Kept RTC schedule ownership in runtime by adding explicit apply helper:
  - `applyRtcScheduleChannels(...)`

- Added native service-level tests:
  - `test/test_v3_config_service/test_main.cpp`
  - coverage includes:
    - invalid API version rejection
    - valid DI/DO payload normalization success

- Updated storage inventory docs:
  - `src/storage/README.md` now lists `v3_config_service.h`.

### Migration Impact

- Shrinks `main.cpp` config-domain responsibility.
- Establishes a clearer storage/config service seam for future modularization and testing.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 32: Move Typed->Legacy Build Helper Out of main.cpp)

### Session Summary

Moved typed-to-legacy build logic (`buildLegacyCardsFromTyped`) out of `main.cpp` into storage config service, with explicit baseline preservation semantics.

### Completed

- Extended storage config service API:
  - `src/storage/v3_config_service.h`
  - added:
    - `buildLegacyCardsFromTypedWithBaseline(...)`

- Implemented helper in:
  - `src/storage/v3_config_service.cpp`
  - behavior:
    - starts from caller-provided baseline cards
    - overlays typed config via `v3CardConfigToLegacy(...)`
    - sanitizes runtime-only fields via `sanitizeConfigCardsRuntimeFields(...)`

- Rewired `main.cpp` callsites:
  - staged save conversion path
  - commit conversion path
  - load conversion path
  - all now call service helper with baseline profile.

- Removed old in-file `buildLegacyCardsFromTyped(...)` implementation from `main.cpp`.

- Added service helper test coverage:
  - `test/test_v3_config_service/test_main.cpp`
  - new test verifies baseline hardware fields are preserved while typed config fields are mapped.

### Migration Impact

- Further reduces config transformation ownership in `main.cpp`.
- Consolidates config transformation behavior inside storage service seam.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 33: Move RTC Schedule Apply Ownership Out of main.cpp)

### Session Summary

Removed `applyRtcScheduleChannels(...)` from `main.cpp` and moved RTC schedule application ownership into storage config service.

### Completed

- Extended storage service API:
  - `src/storage/v3_config_service.h`
  - added:
    - `applyRtcScheduleChannelsFromConfig(...)`

- Implemented schedule apply helper in:
  - `src/storage/v3_config_service.cpp`
  - copies schedule channel fields from normalized output to runtime-owned schedule array.

- Rewired `main.cpp` callsites:
  - staged save/validate, commit, and load flows now call service helper.
  - removed local `applyRtcScheduleChannels(...)` function from `main.cpp`.

- Added service test coverage:
  - `test/test_v3_config_service/test_main.cpp`
  - new test validates field-level copy behavior for RTC schedule channels.

### Migration Impact

- Eliminates another config orchestration helper from `main.cpp`.
- Clarifies schedule application as part of config service boundary.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Config Slice 34: Introduce Typed Config Context Object)

### Session Summary

Introduced a single typed config context object to carry normalized typed cards plus RTC schedule output, and rewired main config flows to consume the context instead of separate buffers.

### Completed

- Added context type in storage config service:
  - `src/storage/v3_config_service.h`
  - `V3ConfigContext` with:
    - `typedCards[]`
    - `typedCount`
    - `rtcChannels[]`
    - `rtcCount`

- Added normalize entrypoint returning context:
  - `normalizeV3ConfigRequestContext(...)` in
    - `src/storage/v3_config_service.h`
    - `src/storage/v3_config_service.cpp`

- Rewired `main.cpp` callsites to context-based flow:
  - staged save
  - staged validate
  - commit
  - load
  - now each path uses one `V3ConfigContext` object and no longer carries separate `typedCards`/`rtcOut` buffers.

- Updated commit path shape:
  - `commitCards(...)` now accepts `const V3ConfigContext&`.

- Extended config service test coverage:
  - `test/test_v3_config_service/test_main.cpp`
  - valid normalize test now calls `normalizeV3ConfigRequestContext(...)` and asserts context content/count.

### Migration Impact

- Removes repeated buffer plumbing from `main.cpp`.
- Establishes a single typed config handoff contract between storage/config service and runtime orchestration.

### Evidence

- Could not run build/tests in this shell because PlatformIO CLI is unavailable (`platformio`, `pio`, and `python -m platformio` not found).
- Acceptance commands to run in your PlatformIO environment:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`

## 2026-03-01 (V3 Runtime Slice 35: Scan Loop Uses Typed Card Config Model)

### Session Summary

Completed scan-path migration so runtime processing dispatch and per-family step config are sourced from typed `V3CardConfig` data (`gActiveTypedCards`) instead of legacy `LogicCard` type/config fields.

### Completed

- Updated runtime-family helpers in `src/main.cpp`:
  - `isDigitalInputCard/isDigitalOutputCard/isAnalogInputCard/isSoftIOCard/isMathCard/isRtcCard`
  - now resolve by `gActiveTypedCards[id].family`.

- Updated runtime scan dispatch in `src/main.cpp`:
  - `processCardById(...)` now switches on `V3CardFamily` from typed active config.

- Migrated per-family scan handlers to typed config sources:
  - `processAICard(...)` uses `V3AiConfig` for channel/range/alpha and runtime index.
  - `processDOCard(...)` uses `V3DoConfig` for condition blocks and mission timing fields.
  - `processSIOCard(...)` uses `V3SioConfig` for condition blocks and mission timing fields.
  - `processMathCard(...)` uses `V3MathConfig` for condition blocks and computation/clamp fields.
  - `processRtcCard(...)` uses `V3RtcConfig` for trigger duration.
  - `processDICard(...)` was already typed in prior in-progress work and now aligns with this slice.

- Updated RTC command path:
  - `setRtcCardStateCommand(...)` now validates RTC by typed family and resolves runtime state by RTC slot index.

### Migration Impact

- Removes scan-time dependence on legacy card type/config fields.
- Keeps `LogicCard` as transitional mirror/state DTO only, not scan behavior owner.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (75/75)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 36: Native Test Portability for Typed Parser/Storage Headers)

### Session Summary

Removed Arduino-framework-only include coupling from typed parser/storage headers so native tests can build parser/normalizer/service flows without requiring `Arduino.h`.

### Completed

- Added compatibility header:
  - `src/kernel/string_compat.h`
  - provides `String` alias for non-Arduino targets while keeping Arduino builds unchanged.

- Updated headers to use compatibility layer:
  - `src/kernel/v3_typed_card_parser.h`
  - `src/storage/v3_normalizer.h`
  - `src/storage/v3_config_service.h`
  - `src/storage/config_lifecycle.h`

### Migration Impact

- Keeps contract/parser/config modules platform-agnostic and testable in native environment.
- Reduces accidental framework coupling in backend logic modules.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (75/75)

## 2026-03-01 (V3 Runtime Slice 37: Fix Config Service Native Test Linkage)

### Session Summary

Resolved `test_v3_config_service` linker failures by explicitly including enum codec implementation in that standalone native harness.

### Completed

- Updated:
  - `test/test_v3_config_service/test_main.cpp`
  - added `#include "../../src/kernel/enum_codec.cpp"` to provide `toString(...)` symbol definitions used by included storage/parser code paths.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (75/75)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 38: Runtime Metadata Refresh Uses Typed Cards)

### Session Summary

Moved runtime metadata refresh to typed card configs, reducing another runtime path that depended on legacy `LogicCard` type/index fields.

### Completed

- Extended runtime metadata module:
  - `src/runtime/runtime_card_meta.h`
  - `src/runtime/runtime_card_meta.cpp`
  - added:
    - `refreshRuntimeCardMetaFromTypedCards(...)`
  - maps typed family to runtime `logicCardType`, derives family index, and resolves per-family mode from typed config.

- Updated runtime sync path in `src/main.cpp`:
  - `syncRuntimeStateFromCards()` now refreshes `gRuntimeCardMeta` from `gActiveTypedCards` via typed metadata refresh API.

- Expanded native test coverage:
  - `test/test_v3_runtime_card_meta/test_main.cpp`
  - added typed metadata refresh test (`DI`, `DO`, `RTC` cases).

### Migration Impact

- Snapshot and runtime signal metadata rebuild now aligns with V3 typed ownership.
- Legacy card array remains transitional mirror/state container only.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (76/76)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (V3 Runtime Slice 39: Typed Runtime Store Sync in Main Path)

### Session Summary

Switched runtime-store synchronization in `main.cpp` from legacy-type dispatch to typed config dispatch.

### Completed

- Updated `syncRuntimeStateFromCards()` in `src/main.cpp`:
  - now calls `syncRuntimeStoreFromTypedCards(logicCards, gActiveTypedCards, ...)`
  - no longer calls legacy `syncRuntimeStoreFromCards(...)`.

### Migration Impact

- Runtime store hydration now follows typed family ownership while preserving legacy runtime-state mirror fields.

## 2026-03-01 (V3 Runtime Slice 40: Typed Mirror API for Runtime->Legacy State Reflection)

### Session Summary

Introduced and adopted typed mirror API to remove runtime reflection dependence on legacy `LogicCard.type`.

### Completed

- Added runtime store API in:
  - `src/kernel/v3_runtime_store.h`
  - `src/kernel/v3_runtime_store.cpp`
  - new:
    - `mirrorRuntimeStoreCardToLegacyByTyped(...)`

- Rewired `src/main.cpp` callsites to typed mirror API:
  - DI/AI/DO/SIO/MATH/RTC process handlers
  - RTC set-state command path
  - runtime sync mirror loop

### Migration Impact

- Runtime reflection uses typed family routing end-to-end in main runtime loop.

## 2026-03-01 (V3 Runtime Slice 41: Typed Runtime Store API Coverage)

### Session Summary

Extended runtime-store tests to cover typed sync and typed mirror entrypoints.

### Completed

- Updated:
  - `test/test_v3_runtime_store/test_main.cpp`
  - added assertions for:
    - typed sync path (`syncRuntimeStoreFromTypedCards`)
    - typed mirror path (`mirrorRuntimeStoreCardToLegacyByTyped`)

## 2026-03-01 (V3 Runtime Slice 42: Remove Legacy Runtime Store Entry Points)

### Session Summary

Removed legacy runtime-store APIs that depended on `LogicCard.type`/`*ForCard` lookups, keeping only index-based and typed entrypoints.

### Completed

- Removed from `src/kernel/v3_runtime_store.h/.cpp`:
  - `syncRuntimeStoreFromCards(...)`
  - `mirrorRuntimeStoreCardToLegacy(...)`
  - `runtime*StateForCard(...)` family wrappers

- Kept:
  - `runtime*StateAt(index, ...)`
  - typed sync/mirror APIs

### Migration Impact

- Eliminates a core legacy dispatch surface from kernel runtime-store module.

## 2026-03-01 (V3 Runtime Slice 43: Remove Legacy Runtime Card Meta APIs)

### Session Summary

Removed legacy card-meta constructors/refresh APIs that accepted `LogicCard[]`, leaving typed card metadata refresh as the canonical path.

### Completed

- Updated:
  - `src/runtime/runtime_card_meta.h`
  - `src/runtime/runtime_card_meta.cpp`
- Removed:
  - `makeRuntimeCardMeta(const LogicCard&)`
  - `refreshRuntimeCardMetaFromCards(...)`

### Migration Impact

- Metadata ownership is now typed-only at the refresh boundary.

## 2026-03-01 (V3 Runtime Slice 44: Runtime Meta/Store Tests Realigned to Typed-Only APIs)

### Session Summary

Completed test realignment to typed-only runtime metadata/store APIs after legacy entrypoint removal.

### Completed

- Updated:
  - `test/test_v3_runtime_store/test_main.cpp`
  - `test/test_v3_runtime_card_meta/test_main.cpp`
- Removed tests targeting deleted legacy APIs.
- Kept/expanded tests for typed-only APIs.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (74/74)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (Cleanup Pass: Post Slice 44 Dead-Code and Include Hygiene)

### Session Summary

Ran a focused cleanup pass after slices 39-44 to remove low-risk dead code and unused includes introduced during migration.

### Completed

- `src/main.cpp`
  - removed unused include:
    - `#include "kernel/v3_runtime_adapters.h"`
  - removed unused typed helper functions:
    - `isSoftIOCard(...)`
    - `isMathCard(...)`
    - `isRtcCard(...)`

- `src/runtime/runtime_card_meta.cpp`
  - removed unused `<stddef.h>` include.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (74/74)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (Cleanup Pass: Legacy Field Alias Seam for Untying Runtime Logic)

### Session Summary

Added an explicit semantic field alias layer for legacy `LogicCard` payload fields (`setting1/2/3`, `startOnMs/startOffMs`) so runtime/config bridge code no longer encodes behavior through raw legacy field names.

### Completed

- Added:
  - `src/kernel/legacy_card_fields.h`
  - semantic getters/setters for DI/DO/AI/MATH/RTC legacy-backed fields.

- Updated callsites to consume semantic aliases:
  - `src/kernel/v3_card_bridge.cpp`
  - `src/kernel/v3_runtime_adapters.cpp`

### Migration Impact

- Reduces direct dependency on legacy generic field names in runtime bridge/adapters.
- Establishes a controlled seam for future replacement of `LogicCard` internal storage without touching per-family runtime logic callsites.

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (74/74)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

## 2026-03-01 (Cleanup Pass: Main.cpp Size Reduction via Legacy Profile + Validator Extraction)

### Session Summary

Reduced `main.cpp` by extracting large legacy card profile/validation blocks into dedicated kernel modules.

### Completed

- Added legacy profile module:
  - `src/kernel/legacy_card_profile.h`
  - `src/kernel/legacy_card_profile.cpp`
  - moved:
    - legacy card JSON serialize/deserialize behavior
    - safe-default card initialization behavior
    - legacy cards-array deserialize behavior

- Added legacy config validator module:
  - `src/kernel/legacy_config_validator.h`
  - `src/kernel/legacy_config_validator.cpp`
  - moved:
    - `validateConfigCardsArray(...)` implementation

- Updated `src/main.cpp`:
  - now delegates those behaviors to module APIs
  - removed in-file long implementation blocks.

### Measured Result

- `src/main.cpp` line count:
  - before this pass: `2426`
  - after this pass: `2201`

### Evidence

- Native tests:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
  - Result: `PASSED` (74/74)

- Firmware build:
  - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
  - Result: `SUCCESS`

### Next Session Start Point

1. Extract HTTP/config route handlers from `src/main.cpp` into `src/portal/routes.cpp` (first target: `/api/config/*` handlers).
2. Move route registration wiring into portal module so `main.cpp` only calls one portal-init entrypoint.
3. Keep behavior identical; no payload contract changes in this slice.
4. Re-run acceptance evidence after extraction:
   - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe test -e native`
   - `C:\Users\Admin\.platformio\penv\Scripts\platformio.exe run`
5. Record resulting `main.cpp` line count delta in this worklog.

## 2026-03-02 (V3 Fast-Track Restart: Branch Freeze + M1/M2 Baseline Scaffold)

### Session Summary

Switched from incremental legacy-constrained updates to a dedicated V3 fast-track branch flow, then implemented milestone baseline scaffolding with typed config validation at boot boundary.

### Completed

- Git safety baseline:
  - created branch `legacy-stable`
  - created working branch `v3-core`
  - created tag `v2-legacy-baseline-2026-03-02`

- Milestone 1 scaffold:
  - replaced monolithic `src/main.cpp` with composition-root wiring
  - added service scaffolding:
    - `src/kernel/kernel_service.*`
    - `src/runtime/runtime_service.*`
    - `src/control/control_service.*`
    - `src/storage/storage_service.*`
    - `src/portal/portal_service.*`
    - `src/platform/platform_service.*`

- Milestone 2 baseline contract path:
  - added typed config contract:
    - `src/storage/v3_config_contract.*`
  - added validator + explicit error codes:
    - `src/storage/v3_config_validator.*`
  - updated storage service to produce/hold validated active config
  - updated kernel service to accept only validated config at `begin(...)`
  - updated bootstrap to fail-fast on invalid config before kernel start

### Why This Slice

- Remove legacy monolith coupling from initial V3 execution path.
- Establish explicit ownership/order at boot: `storage -> validate -> kernel`.
- Keep architecture traceable and aligned with V3 contract boundaries while preserving a rollback-safe legacy baseline.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:01:22.944`
  - RAM: `9.4%` (`30760 / 327680`)
  - Flash: `21.2%` (`278041 / 1310720`)

## 2026-03-02 (M3: JSON Decode Path Into Typed Config Validation)

### Session Summary

Resumed fast-track path and added the bootstrap JSON decode stage so storage can ingest V3 config payloads into typed `SystemConfig` before validation and kernel startup.

### Completed

- Added decode module:
  - `src/storage/v3_config_decoder.h`
  - `src/storage/v3_config_decoder.cpp`
- Added payload-shape error codes:
  - `config_payload_invalid_json`
  - `config_payload_invalid_shape`
  - `config_payload_unknown_family`
  - files:
    - `src/storage/v3_config_validator.h`
    - `src/storage/v3_config_validator.cpp`
- Updated storage bootstrap flow (`src/storage/storage_service.cpp`):
  - attempt load from `/config_v3.json` (LittleFS)
  - deserialize JSON
  - decode JSON into `SystemConfig`
  - validate via `validateSystemConfig(...)`
  - activate only validated config
  - fail bootstrap if file exists but decode/validation fails
  - fallback to default config when file is absent

### Why This Slice

- Makes config ingestion explicit and typed before validator/runtime boundary.
- Removes implicit assumptions that bootstrap config is always internal-only/default.
- Keeps startup gate deterministic with machine-readable failure reason codes.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:01:26.214`
  - RAM: `9.4%` (`30792 / 327680`)
  - Flash: `25.3%` (`331021 / 1310720`)

## 2026-03-02 (M4: Kernel Runtime Binding Summary From Typed Config)

### Session Summary

Completed first runtime-binding slice by making kernel startup consume validated typed config into kernel-owned binding summary metrics (enabled cards and per-family counts).

### Completed

- Updated kernel startup binding:
  - `src/kernel/kernel_service.cpp`
  - computes and stores:
    - `configuredCardCount`
    - `enabledCardCount`
    - `di/do/ai/sio/math/rtc` family counts
- Updated kernel metrics contract:
  - `src/kernel/kernel_service.h`
- Exposed binding summary through runtime snapshot:
  - `src/runtime/runtime_service.h`
  - `src/runtime/runtime_service.cpp`

### Why This Slice

- Makes typed config binding visible and owned by kernel at startup.
- Provides deterministic, inspectable summary for config-to-runtime consistency checks.
- Moves boot path further away from opaque/legacy assumptions.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:00:12.591`
  - RAM: `9.4%` (`30808 / 327680`)
  - Flash: `25.3%` (`331209 / 1310720`)

## 2026-03-02 (M5: Startup Invariant Checks For Binding Summary)

### Session Summary

Started next slice by adding explicit startup invariant checks for typed config binding summary and exposing those checks in runtime snapshot.

### Completed

- Updated kernel metrics:
  - `familyCountSum`
  - `bindingConsistent`
  - file: `src/kernel/kernel_service.h`
- Added startup invariant computation:
  - `familyCountSum == configuredCardCount`
  - `enabledCardCount <= configuredCardCount`
  - file: `src/kernel/kernel_service.cpp`
- Exposed invariant fields in runtime snapshot:
  - `src/runtime/runtime_service.h`
  - `src/runtime/runtime_service.cpp`

### Why This Slice

- Converts binding summary from informational-only to machine-checkable startup invariant state.
- Improves quick detection of config/runtime binding drift during early rewrite milestones.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:00:13.476`
  - RAM: `9.4%` (`30816 / 327680`)
  - Flash: `25.3%` (`331277 / 1310720`)

## 2026-03-02 (M6: Bootstrap Diagnostics In Runtime Snapshot - In Progress)

### Session Summary

Started diagnostics slice to expose storage bootstrap source and error state through runtime snapshot for easier field troubleshooting.

### Completed

- Added storage diagnostics model and API:
  - `BootstrapSource` (`DefaultConfig|FileConfig`)
  - `BootstrapDiagnostics` (`source`, `error`, `hasActiveConfig`)
  - `StorageService::diagnostics()`
  - files:
    - `src/storage/storage_service.h`
    - `src/storage/storage_service.cpp`

- Wired diagnostics to runtime snapshot:
  - new snapshot fields:
    - `bootstrapUsedFileConfig`
    - `storageHasActiveConfig`
    - `storageBootstrapError`
  - files:
    - `src/runtime/runtime_service.h`
    - `src/runtime/runtime_service.cpp`
    - `src/main.cpp` (tick wiring)

### Why This Slice

- Makes startup source and bootstrap error state observable without needing serial-only logs.
- Creates a cleaner troubleshooting path when boot behavior depends on file/default config selection.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:00:15.534`
  - RAM: `9.4%` (`30824 / 327680`)
  - Flash: `25.3%` (`331397 / 1310720`)

## 2026-03-02 (M7: Portal Diagnostics Surface - In Progress)

### Session Summary

Started portal diagnostics slice by introducing a structured portal-facing diagnostics payload built from runtime snapshot binding and bootstrap fields.

### Completed

- Added portal diagnostics state contract:
  - `PortalDiagnosticsState { ready, revision, json }`
  - `PortalService::diagnosticsState()`
  - file: `src/portal/portal_service.h`
- Implemented diagnostics JSON serialization in portal service:
  - file: `src/portal/portal_service.cpp`
  - payload sections:
    - `binding`:
      - `configuredCardCount`
      - `enabledCardCount`
      - `familyCountSum`
      - `consistent`
      - per-family counts (`di/do/ai/sio/math/rtc`)
    - `bootstrap`:
      - `usedFileConfig`
      - `hasActiveConfig`
      - `errorCode` (string from `configErrorCodeToString(...)`)

### Why This Slice

- Converts runtime diagnostics into portal-consumable payload shape.
- Creates a stable handoff point for upcoming HTTP/WebSocket/UI diagnostics rendering.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:00:17.234`
  - RAM: `9.6%` (`31344 / 327680`)
  - Flash: `25.6%` (`335361 / 1310720`)

## 2026-03-02 (M8: Dual-Core Skeleton - In Progress)

### Session Summary

Started dual-core execution skeleton by replacing single-loop scheduling with pinned FreeRTOS task split aligned to V3 ownership model.

### Completed

- `src/main.cpp` dual-core scaffold:
  - Core0 task:
    - function: `core0KernelTask`
    - responsibility: `gKernel.tick(...)` + publish kernel metrics
  - Core1 task:
    - function: `core1ServiceTask`
    - responsibility: `gControl.tick(...)`, `gRuntime.tick(...)`, `gPortal.tick(...)`
  - shared handoff:
    - `gSharedKernelMetrics` protected with `portMUX_TYPE`
    - helper functions `publishSharedKernelMetrics(...)` and `copySharedKernelMetrics()`
  - task creation:
    - `xTaskCreatePinnedToCore(..., core 0)` for kernel task
    - `xTaskCreatePinnedToCore(..., core 1)` for services task
  - failure policy:
    - fail-fast serial message + halt if task creation fails
  - `loop()`:
    - now idle delay only (scheduler work is in tasks)

### Why This Slice

- Aligns runtime execution with V3 core-ownership contract early in skeleton phase.
- Creates a stable concurrency seam before adding queue-based command/snapshot channels.

### Evidence

- Firmware build (user IDE run):
  - Environment: `esp32doit-devkit-v1`
  - Result: `SUCCESS`
  - Duration: `00:00:12.793`
  - RAM: `9.6%` (`31376 / 327680`)
  - Flash: `25.6%` (`335717 / 1310720`)

