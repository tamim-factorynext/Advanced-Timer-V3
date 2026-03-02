# Advanced Timer V3 User Guide (Draft)

Date: 2026-03-02  
Status: Draft reference for end-user documentation.  
Audience: Operators, technicians, and engineers using the completed V3 product.

## 1. DI (Digital Input) Card Guide

This section explains how a DI card works end-to-end: sampling, edge qualification, counter behavior, trigger behavior, and cross-card use.

## 1.1 What A DI Card Provides

A DI card exposes these runtime values:

- `physicalState`: current effective input state after force selection and invert handling.
- `logicalState`: qualified state updated only when an edge passes set/reset and debounce rules.
- `currentValue`: integrated qualified-edge counter.
- `triggerFlag`: one-scan event pulse when a qualified edge increments the counter.
- `state`: internal DI runtime state (`IDLE`, `FILTERING`, `QUALIFIED`, `INHIBITED`).

These values are part of the global runtime variable tree and can be used by other cards where binding/condition rules allow.

## 1.2 DI Configuration Fields

Primary DI settings:

- `channel`: hardware input channel/pin mapping.
- `invert`: whether input polarity is inverted.
- `edgeMode`:
  - `RISING`
  - `FALLING`
  - `CHANGE`
- `debounceMs`: debounce window in milliseconds.
  - must be in 10 ms steps (`0, 10, 20, ...`).
  - `0` means no debounce window (immediate qualification).
- `set` condition block: gate that allows DI processing when true.
- `reset` condition block: dominant reset gate.

Default safe/inert condition setup (recommended baseline):

- `set.clauseA`: `sourceCardId=self`, `operator=ALWAYS_FALSE`
- `set.clauseB`: `sourceCardId=self`, `operator=ALWAYS_FALSE`
- `set.combiner`: `NONE`
- `reset.clauseA`: `sourceCardId=self`, `operator=ALWAYS_FALSE`
- `reset.clauseB`: `sourceCardId=self`, `operator=ALWAYS_FALSE`
- `reset.combiner`: `NONE`

This keeps the card inactive by default until the user intentionally enables logic.

## 1.3 Per-Scan Evaluation Order

Each scan follows this order:

1. Select sample source:
   - forced value (if force mode active), else real hardware input.
2. Apply `invert`.
3. Update `physicalState` from this effective sample.
4. Evaluate `set` and `reset` conditions.
5. Apply gating rules:
   - `reset=true` has top priority.
   - `set=false` blocks edge processing.
6. If processing is allowed, evaluate edge mode and debounce.
7. If edge qualifies, update `logicalState`, increment counter, and pulse `triggerFlag`.

## 1.4 Set/Reset Gating Rules

- If `reset=true`:
  - counter is reset to `0`.
  - `triggerFlag=false`.
  - DI enters inhibited behavior for that scan.
- Else if `set=false`:
  - no edge is processed.
  - no counter increment.
  - counter is not reset.
  - `triggerFlag=false`.
- Else (`set=true` and `reset=false`):
  - edge processing continues.

## 1.5 Edge Mode Rules

- `RISING`: only `0 -> 1` edge can qualify.
- `FALLING`: only `1 -> 0` edge can qualify.
- `CHANGE`: either rising or falling edge can qualify.

If the observed edge does not match mode, no increment occurs.

## 1.6 Debounce Rules

- `debounceMs = 0`:
  - qualifying edge is accepted immediately in that scan.
- `debounceMs > 0`:
  - edge is accepted only if state remains stable for the full debounce window.
  - unstable transitions during the window cancel the pending qualification.

## 1.7 Trigger Flag Behavior

`triggerFlag` is an event pulse:

- set `true` when a qualified edge increments `currentValue`.
- cleared back to `false` on non-trigger scans.

This makes it suitable as a one-scan trigger source for other cards.

## 1.8 Counter Behavior

- Counter increments by `+1` for each qualified edge that passes all rules.
- Counter does not change when:
  - edge does not match mode,
  - set gate is false,
  - debounce is not yet satisfied.
- Counter resets only when reset gate is true.

Counter range and overflow:

- Counter type is unsigned 32-bit (`0 ... 4,294,967,295`).
- Current behavior at max value is wrap-around:
  - if counter is `4,294,967,295`, next qualified increment wraps to `0`.
- This behavior should be considered during long-duration deployments that can accumulate very high edge counts.

## 1.9 Force Input Behavior

DI supports force modes:

- `REAL`: use hardware sample.
- `FORCED_HIGH`: force input sample to high.
- `FORCED_LOW`: force input sample to low.

Force is applied before invert.  
Example: `FORCED_HIGH` with `invert=true` results in effective low.

## 1.10 Practical Examples

Example A: Rising edge counter with debounce

- `edgeMode=RISING`, `debounceMs=50`.
- Input bounces for 20 ms then stabilizes high.
- No increment during bounce; one increment after 50 ms stable high.

Example B: Set gate false

- Input edges occur but `set=false`.
- `physicalState` still updates from effective input.
- `logicalState` and `currentValue` do not advance.

Example C: Reset dominance

- `set=true`, `reset=true` in same scan.
- Reset path wins:
  - counter reset
  - no edge qualification/increment in that scan.

Example D: Trigger pulse usage

- Qualified edge occurs in scan `N`:
  - `triggerFlag=true` in scan `N`.
- No qualified edge in scan `N+1`:
  - `triggerFlag=false` in scan `N+1`.

## 1.11 Operator Notes

- Use `currentValue` for accumulated event counting.
- Use `triggerFlag` for one-scan event triggers.
- Use `physicalState` for immediate effective input visibility.
- Use `logicalState` for debounce-qualified logic transitions.

## 2. WiFi Behavior Guide (Draft)

This section describes intended end-user Wi-Fi behavior for commissioning and recovery.

## 2.1 WiFi Roles

- `Master SSID`: reserved for initial setup and recovery access.
- `User-configured SSID`: normal operational network configured by user.

## 2.2 Intended Connection Strategy

At a high level:

1. Device attempts normal operation using configured user Wi-Fi.
2. If user Wi-Fi is unavailable or misconfigured, user can intentionally trigger
   a recovery path to connect through `Master SSID`.
3. User opens portal over master-network path and updates Wi-Fi settings.

## 2.3 Master SSID Recovery Intent

Master SSID is intended for:

- first-time setup,
- field recovery if user-configured Wi-Fi is not reachable,
- controlled fallback access to reconfigure networking.

Planned user flow (subject to final implementation):

- user enables a hotspot/access point using the known master SSID credentials,
- user restarts device or triggers a recovery action on device,
- device joins master-network path,
- user accesses portal and corrects network configuration.

## 2.4 Recovery Trigger (Future Implementation)

Exact trigger mechanism is not finalized yet.  
Current planned options:

- one dedicated hardware button,
- or a defined button combination/long-press sequence.

This section will be updated after hardware-input mapping is finalized.

## 2.5 Device IP Visibility (Future Implementation)

Final method for showing device IP is still open and will be decided near project completion.

Planned direction:

- onboard display + buttons will be used for live IO status and system pages,
- Wi-Fi/IP status page on that display is expected to provide portal access info.

Until then, this guide does not lock a final "how to discover IP" procedure.

## 3. AI (Analog Input) Card Guide

This section defines current V3 AI behavior (4-20mA assumption for this phase).

## 3.1 What An AI Card Provides

An AI card exposes:

- `currentValue`: filtered and scaled engineering value.
- `state`: AI mission state (currently streaming/continuous).

AI values are part of the global runtime variable tree and can be consumed by other cards through condition logic.

## 3.2 AI Configuration Fields

- `channel`: hardware analog input channel.
- `inputMin` / `inputMax`: input range (assumed 4..20 for 4-20mA cards in this phase).
- `outputMin` / `outputMax`: scaled output range.
- `emaAlphaX100`: filter alpha in centiunit (`0..100` where `100 = 1.00`).

Factory/new-card defaults used by decoder/parser:

- `channel = cardId`
- `inputMin = 4`
- `inputMax = 20`
- `outputMin = 0`
- `outputMax = 100`
- `emaAlphaX100 = 100` (no smoothing)

## 3.3 Per-Scan AI Processing

Each scan:

1. Read raw analog sample from `channel` unless force mode is active.
2. Clamp sample to `[inputMin, inputMax]`.
3. Linearly map clamped value into `[outputMin, outputMax]`.
4. Apply EMA filter:
   - `alpha = emaAlphaX100 / 100`
   - `currentValue = alpha * scaled + (1 - alpha) * previousCurrentValue`
5. Publish `currentValue` to runtime/global signal tree.

## 3.4 Force Behavior

AI supports force commands:

- `setAiForce`: uses provided numeric forced sample as AI input.
- `clearAiForce`: returns to real hardware sample.

Force affects only runtime input selection; it does not change physical wiring/state.

## 3.5 Alpha Behavior Notes

- `emaAlphaX100 = 100`: no smoothing, output follows scaled sample immediately.
- `emaAlphaX100 = 0`: full hold (output stays at previous value).
- valid range is `0..100`; higher values are rejected by config validation.
