# Advanced Timer V4 User Guide (Draft)

Date: 2026-03-05  
Status: Draft imported from V3 and adapted for V4 rebuild track.  
Audience: Operators, technicians, and engineers using the V4 product.

Import note:

- This guide is a V4 working draft seeded from the legacy V3 guide.
- Sections marked draft/proposed remain subject to V4 contract finalization.
- If this guide conflicts with V4 contract docs, the V4 contract docs are authoritative.

## 1. How To Read This Guide

This guide is written around real runtime behavior:

- shared runtime concepts first,
- then card-by-card behavior (`DI`, `AI`, `DO`, `SIO`, `MATH`, `RTC`),
- then device networking/time behavior (`WiFi`, `Time Sync`).

## 2. Shared Runtime Concepts

## 2.1 Scan-Based Execution

The device evaluates logic in repeated scan cycles.  
In each scan, cards read inputs, evaluate conditions, update state, and publish outputs.

## 2.2 Global Runtime Variable Tree

Card outputs are exposed globally and may be referenced by other cards (where allowed by binding rules).  
Common runtime signals:

- `actualState`
- `commandState`
- `edgePulse`
- `liveValue`
- `state` (internal mission/runtime state)

## 2.3 Condition Blocks (`set` / `reset`)

Many cards use `set` and `reset` condition blocks.

- `reset` is dominant over `set` when both are true in the same scan.
- default factory-safe behavior is inert:
  - `set` and `reset` both evaluate to false by default (`ALWAYS_FALSE`, self-referenced).

This keeps cards inactive until intentionally configured.

Naming note:

- Portal/API use `turnOnCondition` and `turnOffCondition` as canonical names.
- `set`/`reset` remain shorthand in this guide for readability.

## 2.5 Numeric Compare Value Source

For numeric condition operators (`GT`, `GTE`, `LT`, `LTE`, `EQ`, `NEQ`), each clause can choose compare value source:

- Constant numeric value (`thresholdValue`)
- Another card's `liveValue` (`useThresholdCard=true`, `thresholdCardId=<card>`)

Rules:

- Only one source is active at a time (`useThresholdCard` selects mode).
- Card-reference compare source is allowed only for numeric operators.
- Referenced threshold card must expose numeric `liveValue` (RTC is not allowed).
- Self-reference in the same clause is rejected.

## 2.4 Counter Range and Overflow

Integrated counters use unsigned 32-bit range (`0 ... 4,294,967,295`).

- At max value, next increment wraps to `0`.
- Plan long-running deployments with wrap behavior in mind.

## 3. DI Card (Digital Input)

## 3.1 What DI Provides

A DI card exposes:

- `actualState`: effective sampled input after force-source selection and invert.
- `commandState`: qualified logical input state.
- `liveValue`: qualified-edge counter.
- `edgePulse`: one-scan pulse when a qualified edge increments counter.
- `state`: DI runtime state (`IDLE`, `FILTERING`, `QUALIFIED`, `INHIBITED`).

## 3.2 DI Configuration Fields

- `channel`
- `invert`
- `edgeMode`: `RISING`, `FALLING`, `CHANGE`
- `debounceMs` (10 ms step rule: `0, 10, 20, ...`)
- `set` condition block
- `reset` condition block
- per numeric clause: `thresholdValue`, `thresholdCardId`, `useThresholdCard`

## 3.3 DI Per-Scan Order

1. Select sample source (`forced` if active, else real hardware).
2. Apply `invert`.
3. Update `actualState`.
4. Evaluate `set` and `reset`.
5. Apply gating (`reset` first, then `set`).
6. If allowed, evaluate edge mode and debounce.
7. On qualified edge, update `commandState`, increment `liveValue`, pulse `edgePulse`.

## 3.4 DI Gating Rules

- `reset=true`:
  - counter reset to `0`
  - `edgePulse=false`
  - qualification blocked in that scan
- `set=false` with `reset=false`:
  - no edge qualification
  - no increment
  - counter not reset
- `set=true` and `reset=false`:
  - normal edge qualification path

## 3.5 DI Debounce and Edge Rules

- `edgeMode=RISING`: only `0 -> 1`
- `edgeMode=FALLING`: only `1 -> 0`
- `edgeMode=CHANGE`: either edge

Debounce:

- `debounceMs=0`: immediate qualification.
- `debounceMs>0`: state must remain stable through full debounce window.

## 3.6 DI Force Modes

- `REAL`
- `FORCED_HIGH`
- `FORCED_LOW`

Force is applied before invert.  
Example: `FORCED_HIGH` with `invert=true` becomes effective low.

## 4. AI Card (Analog Input, 4-20mA Assumption)

## 4.1 What AI Provides

- `liveValue`: scaled + filtered analog value
- `state`: AI runtime state (continuous streaming in current implementation)

## 4.2 AI Configuration Fields

- `channel`
- `inputMin`, `inputMax`
- `outputMin`, `outputMax`
- `smoothingFactorPct` (`0..100`, where `100 = 1.00`)

Factory/new-card defaults:

- `channel = cardId`
- `inputMin = 4`
- `inputMax = 20`
- `outputMin = 0`
- `outputMax = 100`
- `smoothingFactorPct = 100` (no smoothing)

## 4.3 AI Per-Scan Processing

1. Read sample (`forced` if active, else real analog input).
2. Clamp to input range.
3. Scale to output range.
4. Apply EMA filter.
5. Publish `liveValue`.

## 4.4 AI Force Commands

- `setAiForce`: use forced numeric sample.
- `clearAiForce`: return to real hardware.

## 5. DO Card (Digital Output)

## 5.1 DO Purpose

DO executes a timed mission and drives hardware output.

## 5.2 DO Configuration Fields

- `channel`
- `invert` (default `false`)
- `mode`: `Normal`, `Immediate`, `Gated`
- `delayBeforeOnMs`
- `activeDurationMs`
- `repeatCount`
- `set`, `reset` condition blocks
- per numeric clause: `thresholdValue`, `thresholdCardId`, `useThresholdCard`

## 5.3 DO Mission Behavior

- Mission starts when `set=true` and card is retriggerable (`IDLE` or `FINISHED`).
- `reset=true` aborts and returns to idle immediately.
- In `Gated` mode, dropping `set` while running also aborts to idle.
- Retrigger while already running is ignored.

## 5.4 DO Timer Semantics

- `delayBeforeOnMs=0`: no delay phase, immediate active.
- `activeDurationMs=0`: no automatic off timer; stays active until reset (or gated drop).
- If both are `0`: immediate activation with indefinite hold.
- In indefinite hold, repeat cycling is not reached, so `repeatCount` has no practical effect.

## 5.5 DO Invert Semantics

Runtime first calculates mission output, then applies invert:

- `actualState = missionOutput` when `invert=false`
- `actualState = !missionOutput` when `invert=true`

Idle/reset mission output is OFF, so:

- idle/reset physical OFF with `invert=false`
- idle/reset physical ON with `invert=true` (active-low style)

## 5.6 DO Trigger and Counter

- `edgePulse` pulses for one scan on rising edge of final `actualState`.
- `liveValue` increments on the same rising physical edge.

## 6. SIO Card (Soft I/O)

## 6.1 SIO Purpose

SIO follows DO mission/timing behavior but does not drive hardware pins.

## 6.2 SIO Configuration Fields

- `invert` (default `false`)
- `mode`: `Normal`, `Immediate`, `Gated`
- `delayBeforeOnMs`
- `activeDurationMs`
- `repeatCount`
- `set`, `reset` condition blocks
- per numeric clause: `thresholdValue`, `thresholdCardId`, `useThresholdCard`

## 6.3 SIO Runtime Behavior

SIO uses the same mission engine and timing semantics as DO:

- same start/abort/retrigger rules,
- same zero-timer behavior,
- same invert behavior,
- same `edgePulse` and `liveValue` edge rules.

Difference from DO:

- SIO does not issue physical GPIO/relay writes.

## 7. MATH Card (Draft Behavior)

## 7.1 MATH Purpose

MATH computes a numeric value from configured inputs and publishes that value for use by other cards.

MATH is treated as a value-processing card, not an I/O mission card.

## 7.2 What MATH Exposes

MATH exposes:

- `liveValue` only (in centiunits).
- optional `edgePulse` pulse when output value changes in a scan.

MATH does not expose or use:

- `actualState`
- `commandState`
- mission timer phases
- integrated timing counters like DO/SIO

Condition blocks on MATH (when configured) follow the same numeric compare-source rules:

- `thresholdValue` (constant compare value) or
- `thresholdCardId` + `useThresholdCard=true` (compare against another card `liveValue`)

## 7.3 Units and Display

- Internal storage and runtime use integer centiunits.
- Portal should display values as two decimal places (`centi / 100.00`).
- Config/API payloads should carry raw integer centiunits, not float text.
- MATH numeric fields are currently capped to `0..1,000,000` centiunits for the safety envelope (subject to V4 contract finalization).

## 7.4 Proposed Control Behavior

- `reset=true`:
  - force output to `fallbackValue`.
- `set=false` and `reset=false`:
  - hold last output (`liveValue` unchanged).
- `set=true` and `reset=false`:
  - run compute pipeline and update output.

## 7.5 Proposed Compute Pipeline

1. Compute base result from selected operation.
2. Clamp base result to input range:
   - if below `inputMin`, use `inputMin`
   - if above `inputMax`, use `inputMax`
3. Apply range scaling from input range to output range.
4. Apply EMA using `smoothingFactorPct` (`0..100`).
5. Publish final `liveValue` in centiunits.

Operator enum mapping used by backend/config:

- `0 = ADD`
- `1 = SUB_SAT`
- `2 = MUL`
- `3 = DIV_SAFE`

## 7.6 Scaling Direction

- Inverse scaling is allowed.
- Example: `inputMin < inputMax` and `outputMin > outputMax` maps higher input to lower output.
- This is valid and intentional.

## 7.7 Proposed Safety Rules

- No negative values anywhere.
- All MATH fields are unsigned.
- Division-by-zero path should return `fallbackValue`.
- Arithmetic should saturate safely before clamp if intermediate overflow risk appears.
- `edgePulse` should pulse for one scan when output value changes.

## 8. WiFi Behavior (Draft)

## 8.1 WiFi Roles

- `Backup Access Network`: setup/recovery path.
- `User Configured Network`: normal operation path.

## 8.2 Connection Strategy

1. Device tries backupAccessNetwork first with short timeout.
2. If backup network is unavailable, device tries userConfiguredNetwork with longer timeout.
3. If both fail, device enters offline mode while deterministic runtime remains active.
4. Device retries the same sequence in background using non-blocking policy.

## 8.3 Intended Recovery Flow

- user ensures backup access network is available,
- device joins backupAccessNetwork during recovery window,
- user opens portal and updates userConfiguredNetwork settings,
- device reconnects to user network on next connection cycle.

## 8.4 Pending Finalization

Not finalized yet:

- exact physical trigger method for recovery (button mapping/sequence),
- final on-device IP display flow.

Planned direction is to use onboard display + buttons for live status and network info pages.

## 9. RTC Card (Scheduler)

## 9.1 RTC Purpose

RTC is a time scheduler card.  
It does not represent hardware I/O and does not use set/reset condition blocks.

## 9.2 RTC Outputs

RTC exposes:

- `commandState`
- `edgePulse`

RTC does not expose meaningful:

- `actualState`
- `liveValue`

## 9.3 RTC Schedule Fields

Required:

- `minute` (`0..59`)

Optional match groups (enabled by explicit flags):

- `hasHour` + `hour` (`0..23`)
- `hasWeekday` + `weekday` (`0..6`)
- `hasDay` + `day` (`1..31`)
- `hasMonth` + `month` (`1..12`)
- `hasYear` + `year`

Wildcard behavior:

- if `hasX=false`, that field is ignored for matching.
- no negative sentinel values are used.

## 9.4 RTC Trigger Behavior

- When schedule match occurs, `edgePulse` pulses high for one scan.
- At the same moment, `commandState` becomes true.
- `triggerDurationMs` controls how long `commandState` stays true.
- After duration expires, `commandState` returns false.

Retrigger policy:

- `RESTART_WINDOW`: if a new qualified match occurs while already active, active window restarts from the new match time.

## 9.5 Invalid Time Behavior

- If device time is not valid, RTC scheduler does not fire.
- Existing active duration handling remains runtime-local once started.

## 9.6 Disable RTC Card

RTC firing is gated by base card enable:

- `enabled=false` means scheduler never fires.

## 10. Time Sync Settings (Draft)

Time settings are system/global, not per-card.

Planned settings model:

- `timezone` (for local schedule interpretation)
- `timeSync.enabled`
- `timeSync.primaryTimeServer`
- `timeSync.syncIntervalSec`
- `timeSync.startupTimeoutSec`
- `timeSync.maxTimeAgeSec`

These settings are intended to be editable from portal Settings page.



