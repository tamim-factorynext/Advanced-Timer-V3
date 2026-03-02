# Advanced Timer V3 User Guide (Draft)

Date: 2026-03-02  
Status: Draft reference for end-user documentation.  
Audience: Operators, technicians, and engineers using the completed V3 product.

## 1. How To Read This Guide

This guide is written around real runtime behavior:

- shared runtime concepts first,
- then card-by-card behavior (`DI`, `AI`, `DO`, `SIO`),
- then device networking behavior (`WiFi`).

## 2. Shared Runtime Concepts

## 2.1 Scan-Based Execution

The device evaluates logic in repeated scan cycles.  
In each scan, cards read inputs, evaluate conditions, update state, and publish outputs.

## 2.2 Global Runtime Variable Tree

Card outputs are exposed globally and may be referenced by other cards (where allowed by binding rules).  
Common runtime signals:

- `physicalState`
- `logicalState`
- `triggerFlag`
- `currentValue`
- `state` (internal mission/runtime state)

## 2.3 Condition Blocks (`set` / `reset`)

Many cards use `set` and `reset` condition blocks.

- `reset` is dominant over `set` when both are true in the same scan.
- default factory-safe behavior is inert:
  - `set` and `reset` both evaluate to false by default (`ALWAYS_FALSE`, self-referenced).

This keeps cards inactive until intentionally configured.

## 2.4 Counter Range and Overflow

Integrated counters use unsigned 32-bit range (`0 ... 4,294,967,295`).

- At max value, next increment wraps to `0`.
- Plan long-running deployments with wrap behavior in mind.

## 3. DI Card (Digital Input)

## 3.1 What DI Provides

A DI card exposes:

- `physicalState`: effective sampled input after force-source selection and invert.
- `logicalState`: qualified logical input state.
- `currentValue`: qualified-edge counter.
- `triggerFlag`: one-scan pulse when a qualified edge increments counter.
- `state`: DI runtime state (`IDLE`, `FILTERING`, `QUALIFIED`, `INHIBITED`).

## 3.2 DI Configuration Fields

- `channel`
- `invert`
- `edgeMode`: `RISING`, `FALLING`, `CHANGE`
- `debounceMs` (10 ms step rule: `0, 10, 20, ...`)
- `set` condition block
- `reset` condition block

## 3.3 DI Per-Scan Order

1. Select sample source (`forced` if active, else real hardware).
2. Apply `invert`.
3. Update `physicalState`.
4. Evaluate `set` and `reset`.
5. Apply gating (`reset` first, then `set`).
6. If allowed, evaluate edge mode and debounce.
7. On qualified edge, update `logicalState`, increment `currentValue`, pulse `triggerFlag`.

## 3.4 DI Gating Rules

- `reset=true`:
  - counter reset to `0`
  - `triggerFlag=false`
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

- `currentValue`: scaled + filtered analog value
- `state`: AI runtime state (continuous streaming in current implementation)

## 4.2 AI Configuration Fields

- `channel`
- `inputMin`, `inputMax`
- `outputMin`, `outputMax`
- `emaAlphaX100` (`0..100`, where `100 = 1.00`)

Factory/new-card defaults:

- `channel = cardId`
- `inputMin = 4`
- `inputMax = 20`
- `outputMin = 0`
- `outputMax = 100`
- `emaAlphaX100 = 100` (no smoothing)

## 4.3 AI Per-Scan Processing

1. Read sample (`forced` if active, else real analog input).
2. Clamp to input range.
3. Scale to output range.
4. Apply EMA filter.
5. Publish `currentValue`.

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

- `physicalState = missionOutput` when `invert=false`
- `physicalState = !missionOutput` when `invert=true`

Idle/reset mission output is OFF, so:

- idle/reset physical OFF with `invert=false`
- idle/reset physical ON with `invert=true` (active-low style)

## 5.6 DO Trigger and Counter

- `triggerFlag` pulses for one scan on rising edge of final `physicalState`.
- `currentValue` increments on the same rising physical edge.

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

## 6.3 SIO Runtime Behavior

SIO uses the same mission engine and timing semantics as DO:

- same start/abort/retrigger rules,
- same zero-timer behavior,
- same invert behavior,
- same `triggerFlag` and `currentValue` edge rules.

Difference from DO:

- SIO does not issue physical GPIO/relay writes.

## 7. WiFi Behavior (Draft)

## 7.1 WiFi Roles

- `Master SSID`: setup/recovery path.
- `User SSID`: normal operation path.

## 7.2 Connection Strategy

1. Device tries user-configured SSID first.
2. If unavailable/misconfigured, user can invoke recovery flow.
3. Recovery uses master SSID path to access portal and fix settings.

## 7.3 Intended Recovery Flow

- user starts hotspot/access path with master credentials,
- user restarts device or triggers recovery action,
- device connects through master path,
- user opens portal and updates user SSID settings.

## 7.4 Pending Finalization

Not finalized yet:

- exact physical trigger method for recovery (button mapping/sequence),
- final on-device IP display flow.

Planned direction is to use onboard display + buttons for live status and network info pages.
