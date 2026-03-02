# Decision Log

Date: 2026-02-28
Purpose: Minimal "vibe-safe" change log for decisions that affect behavior, contracts, or validation.
Status: Active

## 1. When To Add An Entry

Add an entry when any of the following changes:

- Runtime behavior or deterministic semantics.
- API/schema fields or payload meaning.
- Validation rules and accepted/rejected values.
- Safety, rollback, or fault-handling policy.

## 2. Entry Format (Keep It Small)

Use one short entry per decision with this structure:

```md
## DEC-XXXX: Short Title
- Date: YYYY-MM-DD
- Status: Proposed | Accepted | Superseded
- Context: one paragraph
- Decision: one paragraph
- Impact: 2-5 bullets (behavior, tests, migration, docs)
- References: list of touched files/sections
```

## 3. Workflow Rules

- Contract-first: update docs before or with implementation.
- One decision per intent: avoid bundling unrelated choices.
- Link every behavior change to at least one decision ID.
- If a decision is replaced, keep history and mark old one `Superseded`.

## 4. Decisions

## DEC-0001: Expose Per-Card Evaluation Timing In Runtime Snapshot
- Date: 2026-02-28
- Status: Accepted
- Context: Runtime snapshots already include `lastEvalUs` in examples, but the field intent was not explicitly documented.
- Decision: Keep `lastEvalUs` as a standard per-card runtime snapshot field to expose card evaluation duration in microseconds.
- Impact: Supports deterministic observability and timing regression detection; enables tooling/UI to detect outlier cards without recomputing runtime internals; keeps this field runtime-only and out of config payload requirements.
- References: `docs/api-contract-v3.md` (Section 5.1 rules), `requirements-v3-contract.md` (artifact set and change-control linkage), `docs/legacy/v2-poc-contract.md` (working method linkage).

## DEC-0002: Compile-Time Family Capacities Across All Card Types
- Date: 2026-02-28
- Status: Accepted
- Context: Product models will vary by available channels and optional capabilities; all card families (physical and virtual) must follow one consistent profile-capacity model.
- Decision: Define explicit compile-time capacities for every family (`DI`, `DO`, `AI`, `SIO`, `MATH`, `RTC` alarm channels), allowing `0..N` instances per family by active hardware profile.
- Impact: Enables deterministic multi-model product line support; removes assumptions that any family must exist; formalizes RTC as schedule-alarm channel capacity rather than a special-case family.
- References: `docs/hardware-profile-v3.md`, `requirements-v3-contract.md` (Sections 6.4, 7.1, 8.6), `docs/schema-v3.md` (family presence/capacity), `docs/acceptance-matrix-v3.md` (`AT-HW-005..007`).

## DEC-0003: RTC Stack Baseline (`RTClib` + `RTCMillis` + NTP Sync)
- Date: 2026-02-28
- Status: Accepted
- Context: RTC scheduler behavior must be implemented now without DS3231 hardware dependency, while preserving a clean path to future RTC IC migration.
- Decision: Use Adafruit `RTClib` as the RTC abstraction baseline; use `RTCMillis` as current runtime clock source, and synchronize wall time from NTP. Future hardware migration target is DS3231 using the same `RTClib` API surface.
- Impact: Unblocks RTC scheduler implementation immediately with stable library interfaces.
- Impact: Keeps migration cost low by preserving one RTC library contract across `RTCMillis` and DS3231 backends.
- Impact: Requires deterministic time-sync handling policy when NTP is unavailable or stale.
- References: `platformio.ini`, `docs/hardware-profile-v3.md`, `requirements-v3-contract.md` (RTC sections).

## DEC-0004: Initial V3 Bring-Up Capacity for New Families
- Date: 2026-02-28
- Status: Accepted
- Context: Early implementation needs a small, concrete non-empty target for new card families without introducing unused placeholder card slots.
- Decision: For current bring-up scope, target exactly 2 `RTC` scheduler cards and 2 `MATH` cards; do not reserve additional empty card indices for these families during this phase.
- Impact: Provides a focused implementation/test surface for new families.
- Impact: Simplifies acceptance and debug during early V3 card-family rollout.
- Impact: Does not supersede profile-based `0..N` capacity model; this is an initial bring-up target only.
- References: `docs/hardware-profile-v3.md`, `docs/acceptance-matrix-v3.md`, `docs/worklog.md`.

## DEC-0005: RTC Schedule Granularity Is Minute-Level Only
- Date: 2026-02-28
- Status: Accepted
- Context: RTC scheduler is an alarm-style trigger and does not need second/millisecond scheduling precision in current V3 scope.
- Decision: RTC schedule configuration is limited to minute-level precision. `second` and any millisecond-level schedule fields are not supported and must be rejected by validation.
- Impact: Simplifies scheduler UX and validation semantics.
- Impact: Avoids false precision and reduces configuration ambiguity.
- Impact: Trigger matching must evaluate on minute boundaries (`year/month/day/hour/minute/weekday` only).
- References: `requirements-v3-contract.md` (Section 8.6), `docs/schema-v3.md` (RTC schema and validation), `docs/acceptance-matrix-v3.md` (RTC acceptance cases).

## DEC-0006: RTC Schedule Evaluation Ownership On Networking Core
- Date: 2026-03-01
- Status: Accepted
- Context: RTC schedule matching is minute-granularity and does not require per-scan deterministic execution, while scheduler-card capacity can grow significantly (for example 24 cards).
- Decision: Run RTC schedule-time checks on networking core only, at a bounded once-per-minute cadence, then deliver state-change intents through the runtime command boundary.
- Impact: Preserves deterministic scan-loop budget on Core0.
- Impact: Scales scheduler-card count without coupling minute-tick work to per-scan logic.
- Impact: Requires queue-path acceptance coverage for minute-tick intent delivery and idempotency.
- References: `README.md` (Sections 4.2, 8), `requirements-v3-contract.md` (Core ownership and determinism sections), `docs/timing-budget-v3.md`.

## DEC-0007: Non-Deterministic Service Ownership On Networking Core
- Date: 2026-03-01
- Status: Accepted
- Context: Future roadmap includes services such as logging, remote monitoring/control, periodic non-critical sampling, and Modbus RTU master behavior that can introduce variable latency.
- Decision: All non-time-critical and non-deterministic services must execute on networking core and interact with deterministic state only via defined control/data contracts.
- Impact: Protects deterministic runtime from service-side jitter.
- Impact: Enables feature growth without violating kernel timing guarantees.
- Impact: Sets architectural guardrails for future plugin/service modules.
- References: `README.md` (Section 4.3), `requirements-v3-contract.md` (architecture and topology sections), `docs/dependency-topology-rules.md`.

## DEC-0008: HIL Rig Baseline Uses Raspberry Pi As Test Master
- Date: 2026-03-01
- Status: Accepted
- Context: Firmware validation needs one repeatable rig for functional, stress/performance, and production test flows across deterministic engine and portal/API surfaces.
- Decision: Use Raspberry Pi as the primary HIL orchestrator and production test master; optionally add ESP32 helper nodes for electrical/protocol edge-case signal simulation.
- Impact: Unifies lab and production test automation with one control plane.
- Impact: Improves scriptability, logging, artifact retention, and station management.
- Impact: Preserves low-cost MCU-based signal emulation where it adds realism.
- References: `README.md` (Section 8), `docs/acceptance-matrix-v3.md`, `docs/timing-budget-v3.md`.

## DEC-0009: Remove Dedicated `RUN_SLOW` Mode
- Date: 2026-03-01
- Status: Accepted
- Context: Scan-speed variation is already modeled by configurable scan interval, so a dedicated slow run mode duplicates behavior and increases API/runtime complexity.
- Decision: Remove `RUN_SLOW` from runtime mode enum and command handling. Normal-mode pacing is controlled only through configurable scan interval (portal slider UX).
- Impact: Simplifies run-mode state machine and command validation.
- Impact: `set_run_mode` accepts only `RUN_NORMAL|RUN_STEP|RUN_BREAKPOINT`.
- Impact: Runtime pacing path now uses configured `scanIntervalMs` without slow-mode overrides.
- References: `src/control/command_dto.h`, `src/main.cpp`, `src/kernel/enum_codec.cpp`, `README.md`, `requirements-v3-contract.md` (Section 6.1/6.3), `docs/api-contract-v3.md`.

## DEC-0010: Promote MATH/RTC To First-Class Runtime Families
- Date: 2026-03-01
- Status: Accepted
- Context: Runtime enums already listed `MATH`/`RTC`, but card allocation, scan dispatch, and validation still treated the system as legacy `DI/DO/AI/SIO` only.
- Decision: Add fixed bring-up capacities (`MATH=2`, `RTC=2`) to runtime card allocation, enforce fixed family slot mapping by `cardId`, and process `MATH`/`RTC` directly in scan dispatch.
- Impact: Removes legacy-only card-count assumptions from runtime ownership path.
- Impact: Makes snapshots/config validation aware of all active V3 families in current bring-up scope.
- Impact: RTC set/reset misuse is now rejected during validation.
- References: `src/main.cpp`, `requirements-v3-contract.md` (Sections 6.2, 7.1, 8.5, 8.6), `docs/hardware-profile-v3.md`, `docs/acceptance-matrix-v3.md`.

## DEC-0011: V3 Config Envelope Normalization With Time-Bounded Legacy Bridge
- Date: 2026-03-01
- Status: Accepted
- Context: Config endpoints accepted legacy payloads directly and did not enforce V3 `apiVersion`/`schemaVersion` semantics, creating drift from the frozen API/schema contract.
- Decision: Route staged save/validate/commit through a single normalization path that enforces V3 envelope rules, supports canonical V3 card payload shape, and temporarily bridges legacy card payload shape into the internal model with explicit bridge metadata.
- Impact: Aligns endpoint behavior with V3 contract-first workflow while keeping migration risk controlled.
- Impact: Provides stable machine-readable error codes for unsupported API/schema versions and validation failures.
- Impact: Keeps legacy compatibility explicitly transitional (`usedLegacyCardsBridge`).
- References: `src/main.cpp`, `docs/api-contract-v3.md`, `docs/schema-v3.md`, `requirements-v3-contract.md` (Sections 11, 15, 21).

## DEC-0012: Persist Active/Staged Config In Native V3 Envelope
- Date: 2026-03-01
- Status: Accepted
- Context: Runtime commit path still persisted legacy card arrays, which blocked full migration to the V3 config model and kept storage/API semantics inconsistent.
- Decision: Persist config artifacts as V3 envelope objects with typed `config.cards` payload (`cardId/cardType/config`), and keep backward-compatible legacy-array load fallback for migration safety.
- Impact: Active/staged/factory config files now align with V3 schema direction.
- Impact: Bootstrap/load supports both V3 and legacy storage during transition.
- Impact: Runtime engine still consumes deterministic internal card model via normalization bridge.
- References: `src/main.cpp`, `docs/schema-v3.md`, `docs/api-contract-v3.md`, `requirements-v3-contract.md` (Sections 11, 12, 21).

## DEC-0013: Introduce Per-Family Typed V3 Config Structs Behind Bridge
- Date: 2026-03-01
- Status: Accepted
- Context: A unified legacy `LogicCard` struct remains too broad for six families with divergent schema needs, but runtime migration still requires deterministic, low-risk transition.
- Decision: Add typed per-family V3 config structs (`DI/DO/AI/SIO/MATH/RTC`) and a dedicated bridge (`legacy <-> typed`) as the primary migration seam, while keeping `LogicCard` as transitional runtime DTO.
- Impact: Creates explicit family-specialized model without forcing one-shot runtime rewrite.
- Impact: Reduces future validator/API complexity by centralizing family field ownership.
- Impact: Enables staged removal of legacy-only fields after acceptance coverage is complete.
- References: `src/kernel/v3_card_types.h`, `src/kernel/v3_card_bridge.h`, `src/kernel/v3_card_bridge.cpp`, `src/main.cpp`.

## DEC-0014: Enforce Per-Family Condition Source Field Applicability
- Date: 2026-03-01
- Status: Accepted
- Context: Condition evaluation references only a subset of runtime outputs (`logicalState`, `physicalState`, `triggerFlag`, `currentValue`, `missionState`), and not all fields are valid for all card families.
- Decision: Validate V3 condition clauses against source-card family capabilities before bridging to legacy operators, including strict `missionState` rules (`DO/SIO` only, `EQ` only).
- Impact: Prevents semantically invalid cross-family condition references from entering runtime.
- Impact: Aligns config validation with typed per-family state ownership.
- Impact: Reduces hidden behavior drift caused by permissive clause parsing.
- References: `src/main.cpp` (V3 clause mapping/normalization), `src/kernel/v3_card_types.h`, `docs/schema-v3.md`, `requirements-v3-contract.md` (Section 8).

## DEC-0015: Add Native Acceptance Tests For Condition Field/Operator Rules
- Date: 2026-03-01
- Status: Accepted
- Context: Migration safety needs executable evidence that cross-family condition misuse is rejected consistently, independent of target hardware.
- Decision: Add a native Unity test suite for V3 condition source-field/operator rule helpers and keep a dedicated `native` PlatformIO test environment.
- Impact: Provides fast CI-style guardrails for core validation semantics (`AI.logicalState` reject, `RTC.missionState` reject, `missionState` operator constraints).
- Impact: Reduces regression risk while runtime remains bridge-based.
- References: `src/kernel/v3_condition_rules.h`, `src/kernel/v3_condition_rules.cpp`, `test/test_v3_condition_rules/test_main.cpp`, `platformio.ini`.

## DEC-0016: Validate Into Typed V3 Model Before Legacy Runtime Conversion
- Date: 2026-03-01
- Status: Accepted
- Context: Normalization previously mapped V3 JSON directly into transitional `LogicCard`, which blurred schema validation and runtime adaptation responsibilities.
- Decision: Parse and validate V3 payload cards into typed `V3CardConfig` first, then convert typed cards to `LogicCard` only at the final bridge step.
- Impact: Strengthens contract-first migration boundaries and makes typed model the primary validation target.
- Impact: Reduces risk of legacy-field leakage during schema/API evolution.
- Impact: Keeps deterministic runtime compatibility while migration continues.
- References: `src/main.cpp` (`parseV3CardToTyped`, `buildLegacyCardsFromV3Cards`), `src/kernel/v3_card_types.h`, `src/kernel/v3_card_bridge.cpp`.

## DEC-0017: Add Payload-Level JSON Fixture Validation For Clause Source Rules
- Date: 2026-03-01
- Status: Accepted
- Context: Helper-level tests exist, but migration risk remains unless full payload shape is validated at normalize/parse stage with realistic JSON fixtures.
- Decision: Add `v3_payload_rules` module and native fixture tests that validate cross-family clause source field/operator constraints before typed-card parsing/bridge conversion.
- Impact: Provides acceptance evidence at payload boundary (`config.cards[*].config.set/reset`), not only utility-function scope.
- Impact: Prevents invalid payloads from reaching typed parse/legacy bridge stages.
- Impact: Tightens contract-first behavior for V3 config lifecycle endpoints.
- References: `src/kernel/v3_payload_rules.h`, `src/kernel/v3_payload_rules.cpp`, `test/test_v3_payload_parse/test_main.cpp`, `src/main.cpp`.

## DEC-0018: Move V3 Normalize/Bridge Pipeline Into Storage Module
- Date: 2026-03-01
- Status: Accepted
- Context: `main.cpp` carried too much config-normalization and bridge orchestration logic, weakening core ownership boundaries and making migration harder to reason about.
- Decision: Extract normalize/bridge orchestration into `src/storage/v3_normalizer.*` and keep `main.cpp` normalization entrypoint as a thin boundary wrapper with layout/runtime context wiring.
- Impact: Improves module ownership clarity (`storage` owns config normalization flow).
- Impact: Reduces direct endpoint coupling to conversion internals.
- Impact: Preserves runtime behavior while enabling further decomposition from monolithic `main.cpp`.
- References: `src/storage/v3_normalizer.h`, `src/storage/v3_normalizer.cpp`, `src/main.cpp`, `src/storage/README.md`.

## DEC-0019: Frontend Rebuild Strategy Uses Fresh V3 Portal Baseline
- Date: 2026-03-01
- Status: Accepted
- Context: Portal UI currently reflects legacy-era structure, while backend contracts are being finalized around V3 semantics and legacy bridge removal.
- Decision: Rebuild frontend from a fresh baseline (Bootstrap + icon system + palette/theme options) after backend reaches stable V3-only config/runtime API checkpoint, instead of incrementally patching legacy UI screens.
- Impact: Keeps frontend aligned with V3 contract-first model and avoids carrying legacy UX/DOM constraints into new portal.
- Impact: Reduces long-term maintenance burden by introducing one coherent theme/token system at start of new UI work.
- Impact: Requires explicit parity checklist before retiring old portal routes/pages.
- References: `src/portal/README.md`, `docs/api-contract-v3.md`, `docs/schema-v3.md`, `docs/worklog.md`.

## DEC-0020: Freeze Legacy Baseline And Execute V3 On Dedicated Branch
- Date: 2026-03-02
- Status: Accepted
- Context: Incremental migration on top of legacy patterns was slowing feature delivery and blocking contract-faithful V3 implementation choices.
- Decision: Freeze the existing code line as legacy baseline (`legacy-stable` branch + dated tag) and continue V3 implementation only on dedicated branch `v3-core`.
- Impact: Preserves a known rollback point while allowing larger V3 milestone changes without legacy coupling pressure.
- Impact: Improves decision clarity because V3 architectural changes are no longer constrained by V2 notation debt.
- Impact: Enables milestone-level testing cadence instead of per-micro-change churn.
- References: `git` refs (`legacy-stable`, `v3-core`, `v2-legacy-baseline-2026-03-02`), `docs/INDEX.md`, `requirements-v3-contract.md`.

## DEC-0021: M1/M2 Baseline Uses Composition Root + Validated Typed Config Entry
- Date: 2026-03-02
- Status: Accepted
- Context: A monolithic `main.cpp` obscured ownership boundaries and allowed implicit boot/runtime behavior not anchored to the new V3 contract.
- Decision: Use `main.cpp` as composition root only, scaffold service boundaries (`kernel/runtime/control/storage/portal/platform`), and require `kernel` boot to consume only `ValidatedConfig` from storage validation pipeline.
- Impact: Enforces explicit startup contract (`config -> validate -> kernel begin`) and fails fast on invalid bootstrap config.
- Impact: Establishes clear module seams for upcoming V3 work (JSON decode, schema mapping, runtime binding) without legacy monolith coupling.
- Impact: Keeps architecture aligned with V3 layered boundary rules while maintaining buildability.
- References: `src/main.cpp`, `src/kernel/kernel_service.*`, `src/storage/storage_service.*`, `src/storage/v3_config_contract.*`, `src/storage/v3_config_validator.*`, `requirements-v3-contract.md` (architecture and validation sections).

