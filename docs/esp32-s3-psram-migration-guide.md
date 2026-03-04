# ESP32-S3 V4 Rebuild Plan

Date: 2026-03-04  
Status: Active Plan (execution-ready)

## 1. Intent

Define a full clean rebuild path for `v4` on ESP32-S3 (N16R8-class target), optimized for:

- deterministic runtime behavior
- strong memory management discipline
- low sustained thermal load
- responsive/snappy portal UX
- long-term maintainability

This plan replaces incremental patching strategy on vanilla ESP32 as the primary direction.

## 2. Scope Decision (Locked)

- `v4` is a fresh implementation, not a mass rename of legacy versions.
- Legacy code remains as reference/comparison baseline.
- No planned return to vanilla ESP32 for primary development.
- Legacy compatibility is applied only where it does not harm v4 quality goals.

## 3. Why We Are Rebuilding

Observed pain points on current ESP32 bring-up:

- stack overflows in service task under realistic portal/config traffic
- network/socket pressure symptoms during repeated polling/transport writes
- tight SRAM/flash/LittleFS headroom limiting safe feature growth
- fragile behavior under combined runtime + transport + config workloads

These indicate structural limits for forward scope, not just isolated bugs.

## 4. Design Priorities (Decision Rubric)

Every class/struct/function decision must pass this order:

1. Correctness  
2. Determinism  
3. Safety/robustness  
4. Resource efficiency (CPU, SRAM, PSRAM, flash, network, thermal)  
5. Maintainability  
6. Migration cost

Rule: if an option fails 1-3, reject it regardless of convenience.

## 5. Architecture Direction

### 5.1 Memory Model

- Keep scan-loop hot data in internal SRAM.
- Use PSRAM for large non-real-time buffers (portal payload staging, optional history, debug datasets).
- Avoid unbounded dynamic growth patterns in long-running paths.
- Prefer bounded reusable buffers and explicit capacity budgets.

### 5.2 Runtime and Scheduling

- Keep kernel/control loops deterministic and isolated from heavy transport work.
- Avoid overlapping request handling patterns that can pile sockets.
- Use backpressure-aware queue policies and explicit drop telemetry.

### 5.3 Transport/Portal

- Snapshot and diagnostics contracts are explicit and versioned.
- Polling behavior must be single-flight by default (no request overlap).
- Define endpoint latency targets and payload-size budgets.

### 5.4 Config Pipeline

- Contract-first config schema and validator behavior.
- Validate/save/commit flows are measurable and debuggable.
- Error responses are explicit and machine-parseable.

## 6. Documentation-First Workflow

For every module:

1. Write module contract (`.md`) first.
2. Define acceptance criteria and measurable limits.
3. Implement minimal vertical slice.
4. Add Doxygen for exported symbols.
5. Hardware-test and record findings.
6. Only then expand feature depth.

No large blind coding batch without hardware verification gates.

## 7. Rebuild Phases

### Phase 0: Foundation

- Create dedicated branch (`esp32-s3-v4-rebuild`).
- Establish fresh project skeleton and naming conventions.
- Define root README and architecture map.
- Add build environments for S3 and debug variants.

Exit criteria:

- boots on S3 hardware
- deterministic heartbeat/logging
- baseline CI/build repeatable

### Phase 1: Platform + Storage Core

- platform abstraction (pins, clocks, watchdog, timing, heap telemetry)
- storage layer with robust mount/recovery policy
- config contract v4 draft + parser/validator skeleton

Exit criteria:

- stable boot/reboot loops
- storage recovery behavior verified
- config load/validate path working with typed errors

### Phase 2: Kernel + Runtime Core

- deterministic scan engine
- runtime snapshot model
- queue and telemetry framework

Exit criteria:

- timing envelope stable under synthetic load
- no watchdog/canary faults in soak

### Phase 3: Control + Command Path

- run mode control
- step/simulation primitives
- command queue parity diagnostics

Exit criteria:

- command latency within defined target
- consistent command accounting

### Phase 4: Portal + Config Studio APIs

- `/api/v4/snapshot`, diagnostics, command
- `/api/v4/config/*` active/staged/validate/save/commit/restore
- transport efficiency and connection handling hardening

Exit criteria:

- stable API under prolonged browser polling
- no recurring socket write storms

### Phase 5: Portal UI (Iterative with Backend)

- live page, config studio, settings, learn placeholders
- theme system and icon system from start
- card-centric config UX (not raw JSON UX)

Exit criteria:

- essential flows usable on real hardware
- snappy interactions under normal network conditions

### Phase 6: Simulation/Debug Suite

- stepping/continuous/breakpoints
- clear runtime indicators and safe enable/disable gating

Exit criteria:

- simulation controls reliable and explicitly isolated from production mode

## 8. Non-Functional Targets

### 8.1 Snappiness

- live API response: smooth at 1 Hz polling baseline
- validate/save/commit operations: responsive with clear in-progress state
- no overlapping frontend request storms

### 8.2 Thermal

- reduce unnecessary network/CPU duty during idle observation
- validate 30-minute normal-run thermal trend against baseline

### 8.3 Memory

- explicit budgets for internal SRAM and PSRAM use by subsystem
- fragmentation-aware long-run behavior checks
- no silent fallback on allocation failures

## 9. Acceptance Gates (v4 Go-Live)

Required before declaring v4 production-ready:

- stable boot and 24h soak without resets
- deterministic runtime timing within target envelope
- config API lifecycle robust under repeated edits/validation
- portal transport stable without persistent socket/write errors
- memory telemetry confirms headroom and bounded behavior
- thermal profile acceptable in normal-run scenario

## 10. Risk Register

Risk: scope creep during clean rebuild.  
Mitigation: phase gates + acceptance criteria before expansion.

Risk: overuse of PSRAM for latency-sensitive data.  
Mitigation: strict SRAM/PSRAM placement policy.

Risk: transport regressions under real browser behavior.  
Mitigation: single-flight poll discipline, soak tests, server-side instrumentation.

Risk: docs drift from implementation.  
Mitigation: contract-first + doxygen + phase completion checklist.

## 11. Working Agreement

- We code iteratively with real hardware verification.
- We do not accept "works once" behavior; we require repeatable stability.
- We challenge each design choice with:  
  "Is this the best approach for correctness, determinism, efficiency, and long-term maintainability?"

## 12. Immediate Next Actions

1. Create and switch to `esp32-s3-v4-rebuild` branch.
2. Freeze this plan as migration source of truth.
3. Draft `README.md` for v4 structure and goals.
4. Define first module contracts: platform, storage, config contract.
5. Start Phase 0 implementation.

## 13. Rebuild Watchlist (Project Scan Driven)

Use this list as a standing checklist during v4 implementation reviews.

1. Remove legacy bridge dependencies from runtime-critical paths.
   - Target: typed-only end-to-end config/runtime flow in v4.
2. Split composition root and task loops into smaller orchestrator modules.
   - Target: cleaner boot wiring, explicit scheduler/task policies.
3. Replace repeated JSON/String allocation patterns with bounded memory strategy.
   - Target: explicit allocation policy (SRAM vs PSRAM), capacity budgets, no unbounded growth in service loops.
4. Refactor transport route registration to declarative route tables/middleware.
   - Target: less duplication, consistent error/CORS/telemetry handling.
5. Decouple snapshot/diagnostics generation cadence from service-loop churn.
   - Target: event-driven/rate-limited projection with stable CPU/network footprint.
6. Harden storage lifecycle and first-boot recovery as explicit state machine.
   - Target: deterministic mount/recovery policy with diagnostics counters.
7. Build shared frontend runtime layer instead of duplicating per-page infra.
   - Target: shared API client, shared theming tokens, shared status/error handling.
8. Define transport snappiness policy as contract.
   - Target: single-flight polling, adaptive backoff, no overlapping request storms.
9. Establish test harness before large feature rebuild.
   - Target: unit tests (decode/validate/rules), API contract tests, HIL smoke tests.
10. Enforce module/file complexity limits early.
   - Target: avoid monolithic files in kernel/transport/parser paths.
11. Track resource telemetry continuously during implementation.
   - Target: record Core0/Core1 CPU load, internal SRAM, PSRAM, heap, and per-task stack high-water as first-class progress metrics.
   - Target: keep a running log/history per milestone so regressions are visible early.
