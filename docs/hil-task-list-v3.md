# HIL Task List (V3)

Date: 2026-03-02  
Status: Active checklist for hardware-in-the-loop execution tasks.

## 1. Task Rules

- Keep tasks execution-focused and verifiable.
- Mark each item as `TODO`, `IN_PROGRESS`, or `DONE`.
- Record short evidence after each completed task.

## 2. Task Checklist

## HIL-001: Manual Watchdog Timeout Trigger
- Status: `TODO`
- Goal: verify watchdog timeout/reset path works under controlled fault injection.
- Steps:
  - run firmware with watchdog enabled on both Core0 and Core1 tasks.
  - intentionally stop watchdog reset in one task path (single-core targeted test).
  - observe timeout behavior and confirm expected reset/recovery.
  - capture reboot reason and diagnostics payload before/after recovery.
- Evidence required:
  - serial log snippet showing watchdog-triggered reset.
  - post-reboot diagnostics snapshot confirming recovery state.

## HIL-002: DI Card Contract Verification
- Status: `TODO`
- Goal: verify DI behavior contract (invert/debounce/condition flow) on hardware.
- Steps:
  - load config with representative DI cards (`invert=false/true`, varied debounce).
  - apply controlled digital input transitions (clean edges + bounce pattern).
  - verify snapshot state transitions and debounce timing behavior.
  - verify diagnostics remain stable under rapid edge bursts.
- Evidence required:
  - input stimulus log + matching snapshot transitions.
  - pass/fail note per DI contract scenario.

## HIL-003: DO Card Contract Verification
- Status: `TODO`
- Goal: verify DO timing contract (`onDelayMs`, `onDurationMs`, `repeatCount`) and safe output behavior.
- Steps:
  - load DO timing profiles with short/medium/long timings.
  - trigger output actions from command path and capture GPIO waveform timing.
  - verify repeat cycle count and terminal state handling.
  - confirm behavior under run-mode changes.
- Evidence required:
  - measured waveform timestamps vs configured timings.
  - pass/fail note per DO timing scenario.

## HIL-004: AI Card Contract Verification
- Status: `TODO`
- Goal: verify AI scaling/filter behavior and boundary handling.
- Steps:
  - inject known analog reference points across full input range.
  - verify output scaling (`inputMin/inputMax` -> `outputMin/outputMax`).
  - verify EMA response behavior for step input changes.
  - verify out-of-range handling/fault reporting behavior.
- Evidence required:
  - calibration table (input vs observed output).
  - pass/fail note for scaling/filter/fault scenarios.

## HIL-005: SIO Card Contract Verification
- Status: `TODO`
- Goal: verify SIO pulse/timing behavior parity with configuration.
- Steps:
  - load SIO timing profiles and trigger via expected control path.
  - capture timing waveform for delay/duration/repeat cycles.
  - verify state transitions under rapid retrigger attempts.
- Evidence required:
  - timing capture records and expected vs observed table.
  - pass/fail note for retrigger edge cases.

## HIL-006: MATH Card Contract Verification
- Status: `TODO`
- Goal: verify MATH standard pipeline behavior (`compute -> input clamp -> scale -> ema`) and set/reset control semantics.
- Steps:
  - load MATH configs for each supported operator (`ADD`, `SUB_SAT`, `MUL`, `DIV_SAFE`).
  - verify `reset=true` forces `fallbackValue`.
  - verify `set=false` with `reset=false` holds last output.
  - inject values below/above input range and verify input-side clamp (`inputMin/inputMax`).
  - verify normal and inverse scaling (`outputMin<outputMax`, `outputMin>outputMax`).
  - verify EMA behavior at `emaAlphaX100=100` and a smoothing value (for example `20`).
  - verify divide-by-zero path returns `fallbackValue` deterministically.
  - verify `triggerFlag` pulses for exactly one scan when output changes and stays low when output is unchanged.
- Evidence required:
  - input vector and expected/observed output table (centiunits).
  - pass/fail matrix for operator, clamp, scaling direction, EMA, and trigger-pulse scenarios.

## HIL-007: RTC Card Contract Verification
- Status: `TODO`
- Goal: verify RTC schedule/alarm/channel behavior on device clock.
- Steps:
  - set RTC to controlled reference time.
  - run schedule scenarios across minute/hour/day boundaries.
  - verify activation duration and channel association behavior.
- Evidence required:
  - RTC timeline log with trigger timestamps.
  - pass/fail note for boundary-time scenarios.

## HIL-008: Portal Live Monitoring Verification
- Status: `TODO`
- Goal: verify portal runtime views accurately reflect authoritative firmware snapshot.
- Steps:
  - run multi-card scenario while portal live runtime view is open.
  - compare API snapshot payload against portal-rendered states.
  - verify no optimistic/local recomputation drift in UI values.
- Evidence required:
  - snapshot samples + UI capture references.
  - pass/fail note for rendering parity.

## HIL-009: Portal Simulation/Force Safety Verification
- Status: `TODO`
- Goal: verify simulation/force paths are controlled, observable, and reversible.
- Steps:
  - execute simulation/force actions from portal controls.
  - verify commands follow intended pipeline and are reflected in diagnostics.
  - verify clear return-to-live behavior and state reset.
- Evidence required:
  - command log, diagnostics snapshots, and reset confirmation.
  - pass/fail note for each simulation safety invariant.

## 3. Raspberry Pi Test Rig Backlog

## RIG-001: Raspberry Pi Base Provisioning
- Status: `TODO`
- Goal: prepare reproducible Pi image for AT-V3 HIL station.
- Steps:
  - select and freeze OS version/image.
  - install serial/USB tooling, Python runtime, and logging dependencies.
  - enable boot-time service for test orchestrator.
- Evidence required:
  - provision script and image/version manifest.

## RIG-002: DUT Connectivity Harness
- Status: `TODO`
- Goal: define repeatable wiring and interfaces between Pi and ESP32 DUT.
- Steps:
  - map UART, GPIO stimulus lines, and measurement input lines.
  - document power/reset control method from Pi side.
  - add pinout and safety notes.
- Evidence required:
  - wiring diagram and validated pin map.

## RIG-003: Stimulus/Measurement Module
- Status: `TODO`
- Goal: implement deterministic IO stimulus and capture utilities for card tests.
- Steps:
  - build script/module for digital pulse patterns (clean + bounce profiles).
  - build analog injection/capture workflow (or external module integration).
  - timestamp all stimuli/captures with synchronized clock.
- Evidence required:
  - runnable scripts + sample capture artifacts.

## RIG-004: Automated Test Orchestrator
- Status: `TODO`
- Goal: execute HIL checklist scenarios from one command and store artifacts.
- Steps:
  - map HIL IDs (`HIL-001..`) to executable test routines.
  - define run manifest (selected tests, DUT build ID, config set).
  - store logs/screenshots/captures per run directory.
- Evidence required:
  - orchestrator command reference and sample run output tree.

## RIG-005: CI/Regression Hook (Optional Later)
- Status: `TODO`
- Goal: prepare optional nightly or gated HIL regression trigger.
- Steps:
  - define pass/fail export format.
  - publish summary artifacts for decision logs/worklog linkage.
  - keep manual override for lab safety and hardware availability.
- Evidence required:
  - documented trigger flow and reporting template.
