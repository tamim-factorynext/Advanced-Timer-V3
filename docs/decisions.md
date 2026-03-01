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

