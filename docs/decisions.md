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

## DEC-0022: Bootstrap Config Must Decode To Typed Model Before Validation
- Date: 2026-03-02
- Status: Accepted
- Context: Boot flow initially validated only in-memory default config and lacked a first-class payload decode stage for persisted V3 config JSON.
- Decision: Add dedicated JSON decoder (`v3_config_decoder`) that maps payload -> `SystemConfig`, then run `validateSystemConfig(...)`; kernel starts only from validated typed config.
- Impact: Creates explicit decode/validate separation and clearer failure ownership in storage bootstrap.
- Impact: Adds machine-readable payload error codes for malformed JSON/shape/family mismatches.
- Impact: Prepares path for API/staged-config ingestion to reuse same typed decode+validate pipeline.
- References: `src/storage/v3_config_decoder.*`, `src/storage/v3_config_validator.*`, `src/storage/storage_service.cpp`, `src/main.cpp`.

## DEC-0023: Kernel Boot Publishes Typed Config Binding Summary
- Date: 2026-03-02
- Status: Accepted
- Context: After introducing validated typed-config boot, runtime lacked a kernel-owned checkpoint proving how typed card set was bound at startup.
- Decision: During `KernelService::begin(...)`, compute and store startup binding summary (`configured`, `enabled`, and per-family card counts) from validated typed config, and expose this through runtime snapshot.
- Impact: Adds deterministic observability for config-to-runtime binding at boot.
- Impact: Improves early detection of profile/capacity mismatch regressions.
- Impact: Creates concrete acceptance hooks for future startup invariants.
- References: `src/kernel/kernel_service.h`, `src/kernel/kernel_service.cpp`, `src/runtime/runtime_service.h`, `src/runtime/runtime_service.cpp`, `docs/milestones-v3.md`.

## DEC-0024: Add Startup Binding Invariants To Kernel Metrics
- Date: 2026-03-02
- Status: Accepted
- Context: Binding summary counts were visible but not explicitly marked as consistent/inconsistent, which limited automated sanity checks during startup.
- Decision: Add kernel-owned invariant fields (`familyCountSum`, `bindingConsistent`) derived at startup from validated typed config, and publish them in runtime snapshot.
- Impact: Provides machine-readable startup integrity flag for config-to-runtime binding.
- Impact: Enables acceptance checks to assert binding consistency without duplicating count logic in tests/UI.
- Impact: Reduces ambiguity when diagnosing early boot/config mismatch faults.
- References: `src/kernel/kernel_service.h`, `src/kernel/kernel_service.cpp`, `src/runtime/runtime_service.h`, `src/runtime/runtime_service.cpp`, `docs/milestones-v3.md`.

## DEC-0025: Expose Storage Bootstrap Diagnostics In Runtime Snapshot
- Date: 2026-03-02
- Status: Accepted
- Context: Bootstrap behavior depends on whether config came from persisted file or default fallback, but that state was not visible in runtime snapshot.
- Decision: Add storage diagnostics (`source`, `hasActiveConfig`, `error`) and publish a runtime-facing subset (`bootstrapUsedFileConfig`, `storageHasActiveConfig`, `storageBootstrapError`) each tick.
- Impact: Improves field/debug observability for startup path selection and bootstrap failures.
- Impact: Reduces dependence on serial logs for understanding boot source/error context.
- Impact: Creates stable telemetry hooks for future portal diagnostics panels.
- References: `src/storage/storage_service.h`, `src/storage/storage_service.cpp`, `src/runtime/runtime_service.h`, `src/runtime/runtime_service.cpp`, `src/main.cpp`, `docs/milestones-v3.md`.

## DEC-0026: Portal Diagnostics Payload Mirrors Runtime Binding/Bootstrap State
- Date: 2026-03-02
- Status: Accepted
- Context: Runtime now tracks binding and bootstrap diagnostics, but portal module needed a stable, transport-ready payload contract for commissioning visibility.
- Decision: Add a portal diagnostics state API that maintains serialized JSON payload with `binding` and `bootstrap` sections, derived directly from runtime snapshot each tick.
- Impact: Establishes a clear portal-side diagnostics contract before route/UI wiring.
- Impact: Keeps diagnostics semantics centralized and aligned with runtime source-of-truth fields.
- Impact: Enables incremental addition of HTTP/WebSocket/UI diagnostics without reworking data shape.
- References: `src/portal/portal_service.h`, `src/portal/portal_service.cpp`, `src/runtime/runtime_service.h`, `docs/milestones-v3.md`.

## DEC-0027: Introduce Pinned Dual-Core Task Skeleton In Composition Root
- Date: 2026-03-02
- Status: Accepted
- Context: Skeleton had single-loop execution and did not yet enforce V3 runtime core ownership split.
- Decision: Move scheduling into two pinned FreeRTOS tasks: Core0 runs kernel tick ownership; Core1 runs control/runtime/portal services. Keep cross-core handoff minimal via protected shared kernel metrics while queue channels are not yet introduced.
- Impact: Enforces architectural intent early and reduces future refactor risk for task separation.
- Impact: Provides a minimal, buildable concurrency baseline before queue-based command/snapshot contracts.
- Impact: Requires follow-up milestone for bounded queue channels and timing/latency instrumentation across cores.
- References: `src/main.cpp`, `requirements-v3-contract.md` (core ownership and determinism sections), `docs/milestones-v3.md`.

## DEC-0028: Replace Shared Metrics Copy With Bounded Core0->Core1 Queue
- Date: 2026-03-02
- Status: Accepted
- Context: Dual-core skeleton initially used protected shared-memory copy for kernel metrics handoff, which is weaker than explicit channel boundaries.
- Decision: Introduce a bounded FreeRTOS queue for kernel metrics transport from Core0 producer to Core1 consumer, with non-blocking send and queue health counters (depth/high-water/drop).
- Impact: Improves architectural alignment with queue-driven inter-core boundary model.
- Impact: Provides observable transport behavior under load via queue counters.
- Impact: Creates foundation for extending queue-based contracts to command and snapshot channels.
- References: `src/main.cpp`, `docs/milestones-v3.md`, `requirements-v3-contract.md` (core ownership/channel boundary sections).

## DEC-0029: Introduce Bounded Core1->Core0 Command Queue Skeleton
- Date: 2026-03-02
- Status: Accepted
- Context: After queue-based metrics handoff, command path still lacked an explicit bounded transport boundary from service core to kernel core.
- Decision: Add a bounded FreeRTOS command queue from Core1 to Core0 with non-blocking enqueue, bounded dequeue/apply loop, and command telemetry counters. Use a no-op heartbeat command first to validate channel behavior without altering kernel logic.
- Impact: Establishes command-channel ownership boundary consistent with dual-core architecture contract.
- Impact: Adds observable queue and latency telemetry for future command load tuning.
- Impact: Provides safe migration seam for real kernel command DTO integration in subsequent milestones.
- References: `src/main.cpp`, `docs/milestones-v3.md`, `requirements-v3-contract.md` (core ownership and bounded-channel rules).

## DEC-0030: Promote Core0->Core1 Transport To Snapshot Payload Queue
- Date: 2026-03-02
- Status: Accepted
- Context: Core0->Core1 boundary previously transported metrics only, which limited extensibility for future snapshot channel growth.
- Decision: Replace metrics-only queue with bounded snapshot message queue carrying `producedAtMs` + kernel metrics payload, consumed as latest-wins on Core1.
- Impact: Aligns transport boundary with snapshot-oriented architecture intent.
- Impact: Keeps queue behavior bounded and observable via depth/high-water/drop counters.
- Impact: Simplifies future expansion of snapshot payload fields without redesigning transport type.
- References: `src/main.cpp`, `docs/milestones-v3.md`, `requirements-v3-contract.md` (bounded channels and snapshot semantics).

## DEC-0031: Surface Queue Telemetry In Runtime And Portal Diagnostics
- Date: 2026-03-02
- Status: Accepted
- Context: Queue channels were implemented, but transport health was only available as local counters in composition root and not visible in runtime/portal diagnostics.
- Decision: Add a runtime queue telemetry model and publish snapshot/command queue counters (depth, high-water, drops, command apply/latency) through runtime snapshot and portal diagnostics payload.
- Impact: Enables commissioning and load diagnostics without code-level inspection.
- Impact: Provides early-warning observability for queue saturation and command latency drift.
- Impact: Creates stable telemetry contract for future API/UI diagnostics extensions.
- References: `src/runtime/runtime_service.h`, `src/runtime/runtime_service.cpp`, `src/main.cpp`, `src/portal/portal_service.cpp`, `docs/milestones-v3.md`.

## DEC-0032: Promote Command Queue To Real Run/Step DTO Handling
- Date: 2026-03-02
- Status: Accepted
- Context: Command queue initially validated transport with no-op heartbeat only, which did not prove kernel-control behavior through bounded channel.
- Decision: Use real command DTOs (`KernelCmd_SetRunMode`, `KernelCmd_StepOnce`) on Core1->Core0 queue and apply them via `KernelService` run/step handlers, with command-ack telemetry surfaced in runtime/portal diagnostics.
- Impact: Advances command channel from skeleton transport to behavior-bearing control path.
- Impact: Enables observable command application semantics (counts, last type, latency).
- Impact: Creates direct seam for future API-driven command dispatch without changing core boundary model.
- References: `src/main.cpp`, `src/kernel/kernel_service.h`, `src/kernel/kernel_service.cpp`, `src/runtime/runtime_service.h`, `src/portal/portal_service.cpp`, `docs/milestones-v3.md`.

## DEC-0033: Control Service Becomes Command Source With Validation/Reject Semantics
- Date: 2026-03-02
- Status: Accepted
- Context: Core1 command enqueue path was still generated directly in composition root and lacked a dedicated source boundary for validation and reject-reason tracking.
- Decision: Promote `ControlService` to explicit command-source boundary with bounded pending queue, command request APIs (`setRunMode`, `stepOnce`), reject reasons, and diagnostics; Core1 dispatches control-emitted commands into kernel queue.
- Impact: Separates command intent generation/validation from queue transport mechanics.
- Impact: Adds explicit reject semantics and counters before portal/API command wiring.
- Impact: Improves diagnostics by exposing control-layer pending/request/accept/reject state.
- References: `src/control/control_service.h`, `src/control/control_service.cpp`, `src/main.cpp`, `src/runtime/runtime_service.h`, `src/portal/portal_service.cpp`, `docs/milestones-v3.md`.

## DEC-0034: Portal Command Ingress Queue With Request/Result Mapping
- Date: 2026-03-02
- Status: Accepted
- Context: Command flow had control/kernal queue boundaries but lacked a portal-facing request ingress seam and request-level accept/reject observability.
- Decision: Add a portal ingress queue API for command requests and explicit result recording, then route Core1 dispatch through `Portal -> Control -> Kernel queue`. Surface ingress/request-result counters in portal diagnostics payload.
- Impact: Creates stable pre-transport command entry boundary for upcoming HTTP/WebSocket handler wiring.
- Impact: Provides request-level diagnostics parity (requested/accepted/rejected/reason/last id) independent of transport details.
- Impact: Reduces coupling between portal transport handlers and control validation internals.
- References: `src/portal/portal_service.h`, `src/portal/portal_service.cpp`, `src/main.cpp`, `src/control/control_service.h`, `docs/milestones-v3.md`.

## DEC-0035: Add Transport-Facing Command Submit Result Contract
- Date: 2026-03-02
- Status: Accepted
- Context: Portal command ingress queue existed, but transport handlers needed an immediate, structured response contract without peeking into internal queue state.
- Decision: Introduce portal submit APIs returning `PortalCommandSubmitResult { accepted, reason, requestId }`, with queue-full rejection mapped immediately and ingress counters (`queueAcceptedCount`, `queueRejectedCount`) aligned to submit outcomes.
- Impact: Enables HTTP/WebSocket handlers to return deterministic request-level results at submit time.
- Impact: Clarifies separation between submit acceptance and downstream control/kernel application outcome.
- Impact: Improves diagnostics parity between endpoint response semantics and portal ingress metrics.
- References: `src/portal/portal_service.h`, `src/portal/portal_service.cpp`, `src/main.cpp`, `docs/milestones-v3.md`.

## DEC-0036: Add Thin Transport Command Stub Handlers Over Portal Submit API
- Date: 2026-03-02
- Status: Accepted
- Context: Submit contract existed but transport boundary lacked concrete handler shape for payload parsing and immediate response mapping.
- Decision: Add transport command stub module that parses JSON payloads, supports `setRunMode` and `stepOnce`, routes through portal submit APIs only, and returns immediate structured responses with transport source labeling.
- Impact: Establishes endpoint-ready handler contract without coupling transport code to control/kernel internals.
- Impact: Standardizes immediate response semantics (`accepted/rejected/reason/requestId`) across HTTP/WebSocket entry paths.
- Impact: Reduces integration risk for future real endpoint wiring by validating mapping rules upfront.
- References: `src/portal/transport_command_stub.h`, `src/portal/transport_command_stub.cpp`, `src/portal/portal_service.h`, `docs/milestones-v3.md`.

## DEC-0037: Register Concrete Transport Runtime Endpoints To Stub Handlers
- Date: 2026-03-02
- Status: Accepted
- Context: Transport command stubs existed but were not bound to actual HTTP/WebSocket runtime endpoints in the new skeleton runtime loop.
- Decision: Add a portal transport runtime module that registers concrete endpoints (`POST /api/v3/command`, `GET /api/v3/diagnostics`, WebSocket text command handling on `:81`) and forwards payloads/responses through existing stub handlers and portal diagnostics state.
- Impact: Completes endpoint wiring needed for practical command/diagnostics interaction during skeleton phase.
- Impact: Preserves thin-handler architecture by keeping parsing/mapping centralized in stub layer.
- Impact: Enables next-phase external client integration without changing command flow ownership model.
- References: `src/portal/transport_runtime.h`, `src/portal/transport_runtime.cpp`, `src/main.cpp`, `src/portal/transport_command_stub.cpp`, `docs/milestones-v3.md`.

## DEC-0038: Canonical Transport Command Response Envelope + Status Mapping
- Date: 2026-03-02
- Status: Accepted
- Context: Initial transport responses were functional but lacked fully normalized envelope and error-code/message parity across all failure classes.
- Decision: Enforce one canonical transport response envelope (`ok`, `accepted`, `requestId`, `reason`, `source`, `errorCode`, `message`) with explicit status-code mapping (`200`, `429`, `400`, `422`) and normalized error-code-to-message mapping.
- Impact: Stabilizes external command contract for HTTP/WebSocket clients.
- Impact: Simplifies client-side handling with consistent machine-readable and human-readable fields.
- Impact: Reduces integration regressions caused by inconsistent failure shape/status mapping.
- References: `src/portal/transport_command_stub.cpp`, `src/portal/transport_runtime.cpp`, `docs/milestones-v3.md`.

## DEC-0039: Add Cross-Stage Command Correlation And Parity Flags
- Date: 2026-03-02
- Status: Accepted
- Context: Command flow stages were instrumented individually, but tracing one request across portal ingress, control dispatch, and kernel apply lacked explicit correlation and parity mismatch flags.
- Decision: Propagate request ID through command DTO path, expose portal/control/kernel stage counters together in runtime telemetry, and compute explicit mismatch flags for impossible counter relationships.
- Impact: Enables fast detection of command pipeline drift during commissioning.
- Impact: Improves debuggability by correlating transport request IDs with kernel apply observations.
- Impact: Provides a concrete observability gate for skeleton completion review.
- References: `src/control/control_service.h`, `src/control/control_service.cpp`, `src/main.cpp`, `src/runtime/runtime_service.h`, `src/portal/portal_service.cpp`, `docs/milestones-v3.md`.

## DEC-0040: Skeleton Phase Freeze Gate Achieved
- Date: 2026-03-02
- Status: Accepted
- Context: Skeleton phase required completion of transport binding, command contract hardening, observability parity, and freeze review before moving to feature-complete implementation.
- Decision: Mark skeleton phase complete after `M18`, `M19`, `M20`, and `M21` are all `DONE`, with docs parity and ownership reviews recorded.
- Impact: Establishes a clean transition point from infrastructure scaffolding to behavior/feature implementation phase.
- Impact: Prevents premature “feature phase” start before command/snapshot transport and observability contracts are stabilized.
- Impact: Provides explicit historical checkpoint for future regression triage.
- References: `docs/milestones-v3.md` (Skeleton Completion Gate + M18..M21), `docs/worklog.md` (M21 freeze review), `src/main.cpp`, `src/portal/transport_runtime.cpp`.

## DEC-0041: Add Platform-Owned Per-Task Watchdog Scaffolding
- Date: 2026-03-02
- Status: Accepted
- Context: Requirements include watchdog primitives/fault containment, and post-skeleton phase needed a minimal concrete watchdog integration baseline.
- Decision: Add watchdog primitives in `platform` layer and register/feed Core0/Core1 tasks from composition-root loops with a baseline timeout policy.
- Impact: Introduces practical hang-detection supervision without altering command/snapshot semantics.
- Impact: Preserves architectural ownership (`platform` owns watchdog API; `main` owns task lifecycle/feed points).
- Impact: Creates foundation for future watchdog telemetry and degraded-mode policy integration.
- References: `src/platform/platform_service.h`, `src/platform/platform_service.cpp`, `src/main.cpp`, `requirements-v3-contract.md`, `docs/milestones-v3.md`.

