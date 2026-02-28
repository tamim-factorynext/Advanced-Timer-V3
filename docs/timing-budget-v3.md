# Timing Budget V3

Date: 2026-02-28
Source Contract: `requirements-v3-contract.md` (v3.0.0-draft)
Related: `docs/acceptance-matrix-v3.md`, `docs/api-contract-v3.md`

## 1. Purpose

Define measurable scan timing and command-path timing budgets for V3 runtime validation.

## 2. Scope

Applies to deterministic runtime loop behavior on target hardware for:

- full-scan duration,
- scan overrun detection against active scan interval,
- command queue depth/high-water behavior,
- enqueue-to-apply command latency.

## 3. Runtime Metrics Contract

Runtime snapshot MUST expose at minimum:

- `metrics.scanLastUs`
- `metrics.scanMaxUs`
- `metrics.scanBudgetUs`
- `metrics.scanOverrunLast`
- `metrics.scanOverrunCount`
- `metrics.queueDepth`
- `metrics.queueHighWaterMark`
- `metrics.queueCapacity`
- `metrics.commandLatencyLastUs`
- `metrics.commandLatencyMaxUs`

## 4. Budget Rules

1. `scanBudgetUs` = active scan interval in microseconds (`scanIntervalMs * 1000`).
2. `scanOverrunLast` is `true` if most recent completed full scan exceeded `scanBudgetUs`.
3. `scanOverrunCount` increments once per completed full-scan overrun event.
4. Queue high-water mark must be monotonically non-decreasing until reboot or explicit reset.

## 5. Initial Thresholds (Phase 0 Baseline)

These thresholds are for baseline instrumentation acceptance and may be tightened after hardware profiling:

- Target scan interval (normal mode): `10 ms`.
- Baseline pass threshold under nominal local load:
  - `scanOverrunCount == 0` over 10-minute run.
- Stress visibility threshold under induced portal command burst:
  - `queueHighWaterMark < queueCapacity`
  - no sustained queue growth trend.

## 6. Phase-Gated Tightening

- Phase 0: instrumentation correctness and stable field exposure.
- Phase 3: degraded-mode and fault-containment thresholds defined.
- Phase 4: CI/HIL pass-fail gates frozen from measured percentile distributions.

## 7. Acceptance Mapping

- `AT-REL-*`: scan overrun and stability under portal load.
- `AT-CORE-*`: deterministic full-scan behavior and run-mode timing integrity.
- `AT-CFG-*`: config changes must not violate timing budget reporting integrity.
