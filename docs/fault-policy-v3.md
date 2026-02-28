# Fault Policy V3

Date: 2026-02-28
Source Contract: `requirements-v3-contract.md` (v3.0.0-draft)
Related: `docs/timing-budget-v3.md`, `docs/acceptance-matrix-v3.md`
Status: Draft for implementation

## 1. Purpose

Define deterministic fault classes, severity behavior, and runtime handling rules for V3 firmware.

## 2. Fault Classes

Minimum supported classes:

- sensor fault
- config validation fault
- storage read/write fault
- communication fault
- IO read/write fault
- queue overflow/backpressure fault
- scan overrun fault
- watchdog fault

## 3. Severity Levels

Each class must map to one of:

- `INFO`
- `WARN`
- `CRITICAL`

## 4. Runtime Behavior by Severity

- `INFO`:
  - record telemetry counter/event
  - continue normal operation
- `WARN`:
  - record telemetry counter/event
  - continue deterministic scan
  - expose warning state in snapshot/system status
- `CRITICAL`:
  - latch fault state
  - force unsafe outputs to configured safe state
  - continue deterministic scan only where safe
  - require explicit acknowledge/clear per policy

## 5. Latching and Recovery

- Critical faults are latched until cleared by policy.
- Reboot reason and cumulative fault counters must persist across reboot.
- Recovery order on boot follows config recovery contract (`active -> LKG -> FACTORY`).

## 6. Required Runtime Observability

Snapshot/system metrics should expose, at minimum:

- fault counters by class
- active latched fault flags
- last reboot reason
- queue depth/high-water and overrun counters
- scan overrun count

## 7. Acceptance Mapping

- `AT-REL-001`: severity policy behavior
- `AT-REL-002`: critical safe-state enforcement + latch
- `AT-REL-003`: reboot/fault counter persistence
