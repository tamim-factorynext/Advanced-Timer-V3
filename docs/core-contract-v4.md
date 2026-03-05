# Core Contract (V4)

Date: 2026-03-05
Status: Draft (active)
Supersedes for active work: V3 runtime-core, Wi-Fi, and card behavior baseline sections

## 1. Purpose and Scope

This is the active implementation contract for V4 runtime behavior on ESP32-S3.
It defines mandatory rules for:

- dual-core runtime architecture and deterministic boundaries
- Wi-Fi connectivity behavior
- card-family behavior and validation rules
- force/mask and set/reset semantics
- numeric representation, ranges, and units
- observability and telemetry obligations

When this document conflicts with legacy V3 docs, this V4 contract is authoritative for V4 implementation.

## 2. V4 Design Intent (Normative)

All implementation choices MUST follow this order:

1. Correctness
2. Determinism
3. Safety and robustness
4. Resource efficiency (CPU, SRAM, PSRAM, flash, network, thermal)
5. Maintainability
6. Migration cost

Any option that fails 1-3 MUST be rejected.

## 3. Runtime Architecture Contract

### 3.1 Dual-Core Ownership

- Core0 MUST run deterministic scan/kernel logic only.
- Core1 MUST run variable-latency services (Wi-Fi, transport, config I/O, diagnostics publishing).
- Core0 logic MUST NOT directly call network/socket/filesystem APIs.
- Cross-core interaction MUST use bounded non-blocking queues.
- Queue/backpressure policy MUST be explicit and instrumented.

### 3.2 Scheduling and Blocking Rules

- Core0 scan loop MUST be periodic and bounded by configured scan interval.
- Core0 MUST avoid unbounded waits and blocking cross-core locks.
- Core1 is allowed variable-latency work but MUST use bounded timeout/backoff behavior.

### 3.3 Module and Folder Architecture Contract

V4 implementation MUST keep explicit module boundaries. Recommended source layout:

- `src/kernel/`: deterministic card evaluation, scan execution, runtime state transitions
- `src/platform/`: hardware adapters, pin/backends, time/watchdog primitives
- `src/storage/`: config persistence, decode/validate, transaction and recovery lifecycle
- `src/runtime/`: snapshot projection, metrics aggregation, diagnostic views
- `src/control/`: command validation, command DTOs, queue submission policies
- `src/transport/` (or `src/portal/transport/`): HTTP/WebSocket ingress/egress adapters
- `src/portal/` (optional split): UI payload shaping and portal-facing orchestration
- `src/app/` (or composition root): boot wiring, task startup, dependency assembly

Boundary ownership rules:

- `kernel` MUST NOT directly depend on transport/network/filesystem modules.
- `transport` MUST NOT mutate kernel state directly; it must route through `control` queue boundaries.
- `storage` MUST own config lifecycle and activation contracts; no side-channel commit shortcuts.
- `runtime` MUST project read models only; it is not a mutation owner.
- `platform` adapters MUST hide backend-specific details from `kernel` card logic.

Forbidden couplings:

- `kernel -> transport`
- `kernel -> storage` direct commit mutation paths
- `transport -> kernel` direct state writes
- `portal/frontend glue -> kernel` direct state writes

Required interaction pattern:

- ingress (`transport`) -> validation (`control`/`storage`) -> queued intents -> deterministic apply (`kernel`) -> snapshot projection (`runtime`) -> egress (`transport`)

## 4. Numeric and Unit Contract

### 4.1 Fixed-Point Representation

- Decimal-capable numeric values MUST use integer centiunits (`stored = value * 100`).
- Kernel/runtime MUST operate on integer values.
- UI/portal MUST handle decimal conversion to/from centiunits.

### 4.2 Range and Sign Constraints

- Numeric values covered by this contract MUST be non-negative.
- Validation MUST reject negative values.

Global ceiling for AI/MATH/counter `liveValue` domain:

- min: `0` centiunits
- max: `100,000,000` centiunits
- display max: `1,000,000.00`

Validation MUST reject values above the ceiling for governed fields.

### 4.3 Standard Units

- `ms` for durations and timers (`debounceMs`, `delayBeforeOnMs`, `activeDurationMs`, `triggerDurationMs`)
- `us` for eval diagnostics (`lastEvalUs`)
- `sec` for next-time/time-sync diagnostics where applicable
- `centiunits` for fixed-point numeric pipelines
- booleans as `0/1` state values

Counter-style `liveValue` outputs (`DI/DO/SIO`) MUST saturate at `100,000,000` centiunits (no wrap in contract-facing value path).

## 5. Card Family Contract

### 5.1 Profile-Gated Families

V4 supports these profile-gated families:

- `DI`
- `AI`
- `DO`
- `SIO`
- `MATH`
- `RTC`

Rules:

- Family capacity is compile-time bounded (`0..N`).
- Disabled/zero-capacity families MUST be rejected at validation.
- Kernel card logic MUST remain hardware-agnostic via adapters.

### 5.1.1 Hardware Profile and Backend Selection

Hardware profile selection MUST be compile-time explicit and adapter-driven.

Required profile constants include:

- platform/profile identity (`DAT_PLATFORM_VARIANT`, `DAT_PROFILE_NAME`)
- logical channel maps (`DAT_DI_GPIO_LIST`, `DAT_DO_GPIO_LIST`, `DAT_AI_GPIO_LIST`)
- family capacities (`DAT_SIO_CAPACITY`, `DAT_MATH_CAPACITY`, `DAT_RTC_CAPACITY`)
- transport pin defaults (`UART/I2C/SPI` profile constants)

Backend selection MUST be profile-configurable, with runtime logic unchanged across backends.

Backend code model:

- `DI/DO`: `0=GPIO_DIRECT`, `1=I2C_EXPANDER`, `2=PLUGIN`
- `AI`: `0=INTERNAL_ADC`, `1=I2C_ADC`, `2=PLUGIN`
- `RTC`: `0=RTC_MILLIS`, `1=DS3231`, `2=PCF8523`, `3=DS1307`, `4=PLUGIN`

Validation/runtime rules:

- unsupported backend code for the active build MUST fail validation/boot profile checks.
- switching backend MUST NOT alter card logic semantics, only adapter path.
- plugin backends MUST pass through the same determinism, timeout, and telemetry constraints as built-in backends.

### 5.2 Shared Card Fields and Runtime Signals

Every card config MUST include:

- `cardId`
- `cardType`
- `enabled`
- `label`
- `faultPolicy`
- family-specific config

Every card runtime state MUST include at least:

- `cardId`
- `health` (`OK|WARN|FAULT`)
- `lastEvalUs`
- `faultCounters`
- `liveValue` (family-authoritative output)

Runtime signal projection by family:

- `DI/DO/SIO`: `commandState`, `actualState`, `edgePulse`, `liveValue` are active family signals.
- `AI`: `liveValue` is authoritative; `commandState`/`actualState`/`edgePulse` are non-authoritative defaults.
- `RTC`: `commandState` and `edgePulse` are authoritative scheduler signals; `actualState`/`liveValue` are non-authoritative defaults.

## 6. Set/Reset Contract

### 6.1 Applicability

- Set/reset logic applies only to `DI`, `DO`, `SIO`, `MATH`.
- `AI` and `RTC` MUST NOT define or evaluate set/reset blocks.
- Validation MUST reject set/reset fields on `AI` and `RTC`.

### 6.2 Condition Block Structure

Each set/reset block MUST contain:

- `clauseA`
- `clauseB`
- `combiner` (`None|AND|OR`)

Rules:

- `None` evaluates only `clauseA`.
- `AND`/`OR` combine `clauseA` + `clauseB`.

### 6.3 Compare-Source and State Rules

Numeric compare source MUST be exactly one of:

- constant threshold value
- another card `liveValue` threshold reference

Validation rules:

- self-reference in same clause is invalid.
- `STATE` compare is allowed only for `DO/SIO missionState` (`IDLE|ACTIVE|FINISHED`).
- `STATE` compare operator MUST be equality.

### 6.4 Per-Family Set/Reset Impact

- `DI`:
  - `reset=true` clears counter/filter state and blocks qualification for that scan.
  - `set=false` with `reset=false` inhibits qualification/increment.
  - `set=true` with `reset=false` enables normal edge qualification/increment.
- `DO`:
  - `reset=true` aborts mission immediately to idle-safe path.
  - `set` behavior is mode-dependent (`Normal/Immediate` edge-triggered, `Gated` level-held).
  - if set and reset are both true, reset wins.
- `SIO`: same set/reset semantics as DO mission engine.
- `MATH`:
  - `reset=true` forces `fallbackValue`.
  - `set=false` holds last output.
  - `set=true` executes compute pipeline.

### 6.5 Presence Rules

- For `DI/DO/SIO/MATH`, both `set` and `reset` blocks MUST be present.
- Missing required block(s) MUST fail validation.
- Factory/default baseline for set/reset-capable families MUST initialize both blocks as inert-false.

## 7. Force and Mask Contract

### 7.1 Scope and Precedence

- Force applies to supported input/value-source paths (`DI`, `AI`, and explicitly permitted command surfaces).
- Mask applies to physical output gating (`DO`) only.

Precedence order:

1. Safety/reset policy
2. Force source/value selection (where supported)
3. Mission/logic evaluation
4. Mask-based physical drive gating (DO only)

### 7.2 Per-Family Rules

- `DI` force modes: `REAL`, `FORCED_HIGH`, `FORCED_LOW`.
  - Force is applied before invert/debounce/edge qualification.
- `AI` force:
  - forced sample value (and clear-force path back to real input)
  - forced sample MUST pass normal AI pipeline (`clamp -> map/scale -> EMA`).
- `DO` mask:
  - `DIRECT` or `MASKED` behavior
  - mask suppresses physical drive only; mission/state/timers/counters continue.
- `SIO` mask is unsupported and MUST be rejected.
- `MATH`/`RTC` output-mask semantics are unsupported.

Validation MUST reject unsupported force/mask operations by family.
Diagnostics MUST expose active force/mask state.

## 8. Variable Binding and Topology Contract

- Bind source modes MUST be `CONSTANT` or `VARIABLE_REF`.
- Allowed reference classes: `BOOL`, `NUMBER`, `TIME_WINDOW`, `STATE` (state only from `DO/SIO missionState`).
- Validation MUST enforce type/range/unit compatibility.
- Dependency topology MUST be acyclic.

Ownership rule:

- Each runtime output has a single owner card/service.
- Validation MUST reject write-binding to another card's owned output variable (for example direct write to another card `liveValue`).

## 9. Card-Specific Contract

### 9.1 DI

Config sub-parameters:

- `channel`
- `invert`
- `edgeMode` (`RISING|FALLING|CHANGE`)
- `debounceMs`
- `set`, `reset`

Runtime behavior:

- sample source selection (forced/real) -> invert -> set/reset gating -> debounce -> edge qualification
- `actualState` follows effective sampled input
- `commandState` follows qualified logic state
- `liveValue` increments on qualified edge under active set gate
- `edgePulse` MUST be exactly one scan on qualified increment event

### 9.2 AI

Config sub-parameters:

- `channel`
- `inputRange.min/max`
- `clampRange.min/max`
- `outputRange.min/max`
- `emaAlpha` (`0..100`)

Runtime behavior:

- `raw -> clamp -> map/scale -> EMA -> liveValue`
- no set/reset semantics
- invalid forced values MUST be rejected

### 9.3 DO

Config sub-parameters:

- `channel`
- `invert`
- `mode` (`Normal|Immediate|Gated`)
- `delayBeforeOnMs`
- `activeDurationMs`
- `repeatCount` (`0` means infinite)
- `set`, `reset`

Runtime behavior:

- mission states: `IDLE|ACTIVE|FINISHED`
- signals: `commandState`, `actualState`, physical drive state, `liveValue`, `edgePulse`
- reset dominates set
- retrigger while active is ignored in `Normal` and `Immediate`
- zero-timer semantics:
  - `delayBeforeOnMs=0` => no delay phase
  - `activeDurationMs=0` => no auto-off (hold until reset/gated drop)
  - both zero => immediate hold behavior
- `liveValue` is completed-cycle counter (ceiling saturation applies)
- `edgePulse` semantics MUST remain one-scan pulse on rising effective output edge

### 9.4 SIO

Config sub-parameters:

- `mode` (`Normal|Immediate|Gated`)
- `delayBeforeOnMs`
- `activeDurationMs`
- `repeatCount` (`0` means infinite)
- `set`, `reset`

Runtime behavior:

- Same mission/timing/set-reset semantics as DO.
- No physical GPIO drive.
- Mask is unsupported.
- Zero-timer semantics MUST exactly match DO behavior.
- `liveValue` is completed-cycle counter (ceiling saturation applies).

### 9.5 MATH

Config sub-parameters:

- `mode` (standard pipeline baseline; PID mode only when profile/contract gated)
- `set`, `reset`
- `fallbackValue`
- pipeline parameters: operator, sources, clamp/scale bounds, `emaAlpha`, optional rate limit
- PID parameters when enabled: gains, limits, source bindings

Runtime behavior:

- reset => fallback
- set false => hold
- set true => compute
- output is `liveValue` (global numeric ceiling applies)
- fault path MUST emit deterministic safe output (`fallbackValue`) and fault signal

### 9.6 RTC

Config sub-parameters:

- schedule object: `minute` required; `hour/weekday/day/month/year` optional wildcard-enabled fields
- `triggerDurationMs`

Runtime behavior:

- authoritative time-source scheduler
- minute-level schedule granularity
- outputs: `commandState`, `edgePulse`, `timeUntilNextStartSec`, `timeUntilNextEndSec`
- no set/reset behavior
- invalid/unsynced time MUST prevent firing
- retrigger policy while active MUST be explicit; default is restart-window behavior

## 10. Wi-Fi Connectivity Contract

- Device MUST run in `WIFI_STA` mode for production runtime.
- Two credential slots are mandatory:
  - `backupAccessNetwork` (recovery path)
  - `userConfiguredNetwork` (primary path)

Boot connection order MUST be:

1. backup network (short timeout)
2. user network (longer timeout)
3. offline mode with periodic non-blocking retries

Offline mode MUST keep deterministic runtime fully functional.
Wi-Fi and transport work MUST NOT interfere with Core0 deterministic timing.

## 11. Observability Contract

Runtime MUST expose at minimum:

- per-core CPU load estimate
- heap/SRAM headroom
- PSRAM usage/headroom (when present)
- per-task stack high-water marks
- flash and LittleFS usage
- queue depth/high-water/drop counters for cross-core and transport queues

Metrics collection and publication MUST remain bounded and MUST NOT break Core0 timing guarantees.

## 12. Configuration Persistence Contract (Split Artifacts)

### 12.1 Artifact Model

V4 persistence uses separate LittleFS JSON artifacts:

- `settings` artifact (system/network/time/profile settings)
- `card_config` artifact (card topology, parameters, bindings)

Unified single-file persistence is not the V4 baseline.

### 12.2 Transaction and Atomicity Rules

- Runtime activation MUST use a logically atomic pair view across `settings` and `card_config`.
- Device MUST NOT activate a mixed pair from different `schemaVersion`/`commitId` revisions.
- Both artifacts MUST carry compatible metadata (`schemaVersion`, `commitId`, compatibility markers/hashes).
- Commit operation MUST validate both artifacts and cross-artifact dependencies before activation.

### 12.3 Boot and Recovery Rules

- If either artifact is missing/invalid/incompatible, boot MUST fall back to last known good compatible pair.
- Deterministic runtime MUST remain safe/offline-capable under persistence recovery paths.
- Recovery events MUST be surfaced in diagnostics counters/logs.
- LKG is an internal robustness mechanism and MUST NOT be exposed as a user rollback target.

### 12.4 Validation Scope

- Settings validation and card-config validation MUST both pass before commit.
- Cross-artifact constraints (for example backend/profile references used by card config) MUST be validated as a single transaction boundary.

### 12.5 User Restore and Portal Action Placement

- User/API restore operations MUST NOT offer "restore LKG".
- User-facing restore targets are limited to factory reset and explicit import payloads.
- Internal runtime/boot recovery MAY auto-activate LKG pair when integrity checks fail.
- Any automatic LKG recovery MUST be reported in diagnostics and boot/recovery logs.
- Portal action placement MUST be:
  - `Config` page: export/import/import-validation/apply-commit config lifecycle actions.
  - `Settings` page: factory reset and system/recovery status visibility.

## 13. Alignment Verdict and Next Improvement Targets

Current verdict: This contract is aligned with V4 goals (determinism, resilience, observability, maintainability).

Improvement targets (next iteration):

1. Add explicit acceptance-test IDs per major clause (traceability to AT/HIL matrix).
2. Split transport/API and config lifecycle details into dedicated V4 contract files and cross-link them here.
3. Add per-field min/max table appendix for all card config parameters to remove parser ambiguity.
