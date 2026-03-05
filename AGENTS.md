# AGENTS Guideline (V4)

Scope: This file governs all meaningful project changes (code, contracts, docs, runtime behavior).

## 1. Core Intent

Build V4 on ESP32-S3 with deterministic runtime, resilient connectivity, strong observability, and maintainable architecture.

## 2. Decision Order (Non-Negotiable)

Evaluate every change in this order:
1. Correctness
2. Determinism
3. Safety/robustness
4. Resource efficiency (CPU, SRAM, PSRAM, flash, network, thermal)
5. Maintainability
6. Migration cost

Reject any option that fails 1-3.

## 3. Mandatory Workflow

For each module/change:
1. Confirm request + success criteria.
2. Check governing contracts/decisions before coding.
3. Prefer minimum safe increment.
4. Implement with clean boundaries.
5. Validate (build/test/hardware/log as applicable).
6. Complete documentation updates as part of done criteria.

No large blind coding batch without verification gates.

## 4. Documentation Is Mandatory

Documentation is part of implementation, not optional cleanup.

1. Worklog: record meaningful progress in `docs/worklog-v4.md`.
2. Decisions: update `docs/decisions.md` when behavior, policy, architecture, or trade-off direction changes.
3. Contracts/specs: update affected contract docs when implementation behavior changes.
4. Code docs: add/maintain Doxygen-style documentation for exported files, public interfaces, and non-obvious logic.
5. Completion rule: a change is not complete until required docs are updated.

## 5. Architecture and Runtime Rules

1. Contract-first: clarify behavior in docs before deep implementation when unclear.
2. Determinism-first: Core0 must remain isolated from blocking and non-deterministic work.
3. Core separation:
   Core0 = deterministic kernel/scan.
   Core1 = Wi-Fi/transport/storage/control and variable-latency work.
4. No direct kernel calls to network/filesystem APIs in timing-critical paths.
5. Cross-core communication must be bounded and non-blocking.
6. Queue/backpressure policy must be explicit and instrumented (including drop telemetry).

## 6. Memory and Performance Rules

1. Keep hot deterministic data in internal SRAM.
2. Use PSRAM for large non-real-time buffers.
3. Avoid unbounded dynamic growth; prefer bounded reusable buffers with explicit capacities.
4. Enforce complexity limits early; avoid monolithic files/loops in kernel and transport paths.
5. Transport must avoid request storms; use single-flight and explicit backoff.

## 7. Telemetry and Verification Rules

1. Telemetry-first is mandatory for progress and regression control.
2. Continuously track and log:
   Core0/Core1 CPU load, SRAM, PSRAM, heap, stack high-water, flash usage, LittleFS usage.
3. Maintain milestone-level telemetry history.
4. In final reports, state:
   what was verified, what remains assumed, and key risks.

## 8. Change Quality Requirements

For non-trivial changes, always provide:
1. Chosen approach
2. At least one alternative considered
3. Trade-off reason for final choice
4. Main risks and mitigation

No silent assumptions.
