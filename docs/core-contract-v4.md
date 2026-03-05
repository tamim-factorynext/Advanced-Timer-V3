# Core Contract (V4)

Date: 2026-03-05
Status: Draft (active)
Supersedes for active work: V3 runtime-core + Wi-Fi sections

## 1. Purpose

Define the V4 runtime core contract for ESP32-S3, with focus on:

- dual-core ownership and scheduling boundaries
- Wi-Fi connectivity behavior and constraints
- card-family behavior contract for deterministic runtime cards

This contract imports V3 baseline rules and applies V4-specific adjustments.

## 2. Imported Baseline (From V3)

Imported from `legacy/requirements-legacy-v3-contract.md`:

- Core ownership model:
  - Core0 owns deterministic kernel state and scan execution.
  - Core1 owns network/portal/storage/control transport.
  - Cross-core communication must use bounded channels.
- Wi-Fi constraints:
  - STA mode only.
  - Non-blocking networking behavior must not interfere with Core0 timing.
- Connectivity fallback:
  - Dual-SSID strategy (backup + user-configured).
  - Boot-time connection sequence with offline-mode fallback.
- Card model baseline:
  - Profile-gated card families (`DI`, `AI`, `DO`, `SIO`, `MATH`, `RTC`).
  - Shared set/reset semantics for stateful cards.
  - Per-card deterministic evaluation rules.

## 3. V4 Core Runtime Contract

### 3.1 Dual-Core Ownership (Mandatory)

- Core0 MUST run deterministic scan/runtime logic only.
- Core1 MUST run non-deterministic services (Wi-Fi, HTTP/WebSocket, config I/O, diagnostics transport).
- Kernel logic on Core0 MUST NOT call Wi-Fi/HTTP/filesystem APIs directly.
- Core1 -> Core0 commands MUST be queued through bounded, non-blocking interfaces.
- Core0 -> Core1 snapshots/events MUST be published through bounded queues with drop/overwrite policy defined in code.

### 3.2 Scheduling and Blocking Rules

- Core0 loop MUST be periodic and bounded by configured scan interval.
- Core0 MUST avoid unbounded waits, socket calls, and filesystem operations.
- Core1 MAY perform variable-latency operations, but MUST use timeouts and retry backoff.
- Any cross-core lock that can block Core0 is forbidden.

### 3.3 Wi-Fi Connectivity Contract (V4)

- Device MUST operate in `WIFI_STA` mode for production runtime.
- Device MUST support two credentials:
  - `backupAccessNetwork` (factory recovery path, non-editable by normal user flow)
  - `userConfiguredNetwork` (primary runtime network)
- Boot connect order MUST be:
  1. backup network (short timeout)
  2. user network (longer timeout)
  3. offline mode with periodic background retries
- Offline mode MUST keep deterministic runtime fully functional.
- Reconnect attempts MUST be non-blocking to Core0 and rate-limited.

### 3.4 Observability Hooks (Required by DEC-0001)

- Runtime MUST expose at least:
  - per-core CPU load estimate
  - heap/SRAM headroom
  - PSRAM usage/headroom (when present)
  - per-task stack high-water marks
  - flash and LittleFS usage
- Metrics collection MUST be low-overhead and interval-based.
- Metrics publishing MUST NOT violate Core0 timing guarantees.

## 4. V4 Modifications vs V3 (Core + Wi-Fi)

- V3 clause "Core1 owns portal/network/filesystem/control transport" is kept, but V4 makes queue/backpressure behavior explicit as a required implementation contract.
- V3 Wi-Fi dual-SSID strategy is kept, but V4 formalizes retry rate-limiting and offline-first continuity as explicit mandatory behavior.
- V4 adds mandatory health telemetry hooks as part of core contract (from `docs/decisions.md`, DEC-0001).

## 5. Alignment Check with V4 Goals

Goal: deterministic ESP32-S3 rebuild with strong observability and resilient connectivity.

- Determinism: aligned
  - strict Core0 isolation from variable-latency subsystems
- Connectivity resilience: aligned
  - retained dual-SSID + offline fallback + retry policy
- Telemetry-first workflow: aligned
  - observability hooks included as contract requirements
- Migration practicality: aligned
  - reuses proven V3 rules, narrows and clarifies for V4 implementation

Verdict: This contract aligns with current V4 goals and can be used as the active implementation baseline for core scheduling and Wi-Fi runtime behavior.

## 6. Card Families Contract (Imported + Adapted)

### 6.1 Profile-Gated Families

Supported card families for V4:

- `DI` (Digital Input)
- `AI` (Analog Input)
- `DO` (Digital Output)
- `SIO` (Soft IO)
- `MATH` (deterministic numeric compute)
- `RTC` (time scheduler)

Rules:

- Every family is optional per build profile.
- Family capacities are compile-time bounded (`0..N`).
- Validation MUST reject card types whose family is disabled or has zero capacity in the active profile.
- Kernel logic MUST stay hardware-agnostic through adapters.

### 6.2 Shared Card Configuration Contract

Every card config MUST include:

- `cardId`
- `cardType`
- `enabled`
- `label`
- `faultPolicy`
- family-specific config block

Every card runtime state MUST include at least:

- `cardId`
- `health` (`OK|WARN|FAULT`)
- `lastEvalUs`
- `faultCounters`
- `liveValue` (authoritative output variable for the family)

Family semantics:

- `DI/DO/SIO` expose mission/stateful behavior.
- `AI` is transducer style and does not use mission semantics.
- `RTC` is schedule evaluator and does not use set/reset condition blocks.

### 6.3 Set/Reset Condition Contract

Set/reset semantics apply to `DI`, `DO`, `SIO`, and `MATH`.

Rules:

- `reset` is dominant over `set` in the same scan.
- For `DO/SIO`, set behavior is mode-dependent:
  - `Normal` and `Immediate`: edge-triggered mission start.
  - `Gated`: level-triggered; set must remain true through mission window.
- `AI` and `RTC` do not evaluate set/reset blocks internally.
- Validation MUST reject `set/reset` blocks on `AI` and `RTC`.

### 6.4 Set/Reset Impact Matrix (Per Card Type)

- `DI`:
  - `reset=true`: counter reset, qualification blocked for that scan.
  - `set=false` with `reset=false`: no qualification/increment.
  - `set=true` with `reset=false`: normal qualification and edge counting.
- `DO`:
  - `reset=true`: immediate mission abort to idle-safe state.
  - `set` trigger behavior depends on mode (`Normal/Immediate` edge-triggered, `Gated` level-held).
  - if both set and reset are true in same scan, reset wins.
- `SIO`:
  - same set/reset semantics as DO mission engine.
  - reset aborts virtual mission immediately.
- `MATH`:
  - `reset=true`: output forced to `fallbackValue`.
  - `set=false` and `reset=false`: hold last output.
  - `set=true` and `reset=false`: execute compute pipeline.
- `AI`:
  - no set/reset behavior exists.
  - any set/reset config for AI is invalid and must be rejected.
- `RTC`:
  - no set/reset behavior exists.
  - state is schedule/time driven only; set/reset config is invalid.

### 6.5 Missing/Unsupported Set/Reset Blocks

- For card families that support set/reset (`DI`, `DO`, `SIO`, `MATH`), both blocks MUST be present in validated config.
- If a card supports set/reset and either block is missing, validation MUST reject the card configuration.
- For card families that do not support set/reset (`AI`, `RTC`), set/reset fields MUST be absent.

## 7. Card-Specific Requirements (V4 Baseline)

### 7.0 Common Units and Value Conventions

- `ms`: milliseconds for timer/duration fields (`delayBeforeOnMs`, `activeDurationMs`, debounce fields).
- `us`: microseconds for internal timing diagnostics (`lastEvalUs`).
- `sec`: seconds for time-until diagnostics and sync settings where specified.
- `centiunits`: scaled integer numeric value (`value * 100`) for deterministic numeric pipelines.
- boolean states: `0/1` semantics for `commandState`/`actualState`.
- counters: unsigned 32-bit (`0..4294967295`, wrap on overflow unless otherwise specified).

### 7.1 DI

- Config sub-parameters:
  - `channel` (digital input channel index)
  - `invert` (`bool`)
  - `edgeMode` (`RISING|FALLING|CHANGE`)
  - `debounceMs` (`ms`, range validated by profile/contract)
  - `set` condition block
  - `reset` condition block
- Runtime outputs/signals:
  - `actualState` (effective sampled input after force + invert)
  - `commandState` (qualified logical state)
  - `liveValue` (qualified edge counter, count)
  - `edgePulse` (one-scan pulse on qualified increment)
- Rules:
  - counter increments only on qualified edges when set gate is active.
  - reset clears counter and dominates set.
  - force transitions must not corrupt edge qualification behavior.

### 7.2 AI

- Config sub-parameters:
  - `channel` (analog input channel index)
  - `inputRange.min`, `inputRange.max` (engineering input domain; unit defined per channel profile)
  - `clampRange.min`, `clampRange.max` (same base unit as input)
  - `outputRange.min`, `outputRange.max` (target engineering unit, typically centiunits)
  - `emaAlpha` (`0..100`, centiunits of filter alpha `0.00..1.00`)
- Runtime pipeline:
  - `raw -> clamp -> map/scale -> EMA -> liveValue`
- Runtime outputs/signals:
  - `liveValue` (scaled filtered numeric output)
  - quality metadata (`GOOD|CLAMPED|INVALID`) when enabled by snapshot contract
- Rules:
  - AI is always transducer/evaluator, not mission-state card.
  - AI MUST NOT include or evaluate `set/reset` style logic.
  - invalid forced values are rejected by command validation.

### 7.3 DO

- Config sub-parameters:
  - `channel` (digital output channel index)
  - `invert` (`bool`)
  - `mode` (`Normal|Immediate|Gated`)
  - `delayBeforeOnMs` (`ms`)
  - `activeDurationMs` (`ms`)
  - `repeatCount` (count; `0` = infinite repeat)
  - `set` condition block
  - `reset` condition block
- Runtime outputs/signals:
  - `missionState` (`IDLE|ACTIVE|FINISHED`)
  - `commandState` (logical mission output before mask/invert handling)
  - `actualState` (effective output state after mode/invert evaluation)
  - physical drive state (after mask policy)
  - `liveValue` (completed-cycle counter)
  - `edgePulse` (one-scan pulse on configured output transition event)
- Rules:
  - reset always aborts mission immediately.
  - masking affects physical drive only, not logical mission evaluation.
  - retrigger while active is ignored in `Normal` and `Immediate`.

### 7.4 SIO

- Config sub-parameters:
  - `mode` (`Normal|Immediate|Gated`)
  - `delayBeforeOnMs` (`ms`)
  - `activeDurationMs` (`ms`)
  - `repeatCount` (count; `0` = infinite repeat)
  - `set` condition block
  - `reset` condition block
- Runtime outputs/signals:
  - `missionState` (`IDLE|ACTIVE|FINISHED`)
  - `commandState`
  - `actualState`
  - `liveValue` (completed-cycle counter)
  - `edgePulse`
- Rules:
  - semantics match DO mission engine and timer/cycle behavior.
  - no physical GPIO drive.
  - SIO mask semantics are not applicable.

### 7.5 MATH

- Config sub-parameters:
  - `mode` (standard pipeline baseline; optional PID by profile/contract gate)
  - `set` condition block
  - `reset` condition block
  - `fallbackValue` (numeric safe output, typically centiunits)
  - standard-pipeline parameters (operator, input sources, clamp/scale ranges, `emaAlpha`, optional rate limit)
  - PID parameters when enabled (gains, output limits, source bindings)
- Runtime outputs/signals:
  - `liveValue` (numeric output, typically centiunits)
  - intermediate stage telemetry (when diagnostics enabled)
  - fault status/flags
- Rules:
  - reset forces `fallbackValue`.
  - set false holds last value.
  - fault paths must produce deterministic safe output (fallback) and fault signal.

### 7.6 RTC

- Config sub-parameters:
  - schedule fields: `minute` required; `hour/weekday/day/month/year` optional wildcard-enabled fields
  - `triggerDurationMs` (`ms`)
- Runtime outputs/signals:
  - `commandState` (active window boolean)
  - `edgePulse` (trigger pulse)
  - `timeUntilNextStartSec`, `timeUntilNextEndSec` (`sec`)
- Rules:
  - schedule-driven boolean source using authoritative system time service.
  - minute-level schedule granularity.
  - no set/reset evaluation.
  - unsynced/invalid time must prevent schedule firing.

## 8. V4 Card Adaptations vs Legacy V3

- Imported V3 family behavior and precedence rules are retained as baseline.
- V4 narrows the contract to deterministic, profile-gated implementation order and explicit runtime observability.
- V4 keeps `MATH` mode expansion possible, but requires mode gates through profile/config contract before enabling in production.
- V4 keeps `RTC` time-service dependency explicit and aligned with the simplified Time Sync direction.

## 9. Force and Mask Behavior Contract

### 9.1 Scope

- Force applies to input/value-source style cards (`DI`, `AI`) and controlled runtime values where explicitly allowed.
- Mask applies to physical output drive behavior (`DO`) only.

### 9.2 Priority and Precedence

- For all cards, `reset` precedence rules remain unchanged (reset dominates set where applicable).
- Force changes data source/value used by card logic.
- Mask changes physical drive behavior only; it must not rewrite logical mission state.
- If multiple control layers apply, precedence is:
  1. safety/reset policy
  2. force source/value selection (where supported)
  3. mission/logic evaluation
  4. mask/physical drive gating (DO only)

### 9.3 Per-Card Rules

- `DI` force:
  - supported modes: `REAL`, `FORCED_HIGH`, `FORCED_LOW`.
  - force is applied before invert/debounce/edge qualification.
  - DI force must not corrupt edge counter integrity across transitions.
- `AI` force:
  - supported mode: forced numeric sample (plus clear force to return to real input).
  - forced sample goes through the same pipeline (`clamp -> map/scale -> EMA`).
  - invalid forced values are rejected.
- `DO` mask:
  - supported states: `DIRECT` and `MASKED` (or equivalent config/command representation).
  - mask suppresses physical output drive only.
  - mission timers, counters, set/reset evaluation, and logical runtime state continue normally while masked.
- `SIO`:
  - no physical drive exists; mask is not applicable and must be rejected if requested.
- `MATH` and `RTC`:
  - output-mask semantics are not applicable.

### 9.4 Validation and Diagnostics

- Validation must reject unsupported force/mask commands by card family.
- Runtime diagnostics should expose current force/mask states so operator intent is auditable.
