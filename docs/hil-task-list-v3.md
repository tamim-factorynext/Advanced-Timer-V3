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
