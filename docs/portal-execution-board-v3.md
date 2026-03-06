# V3 Portal Execution Board

Date: 2026-03-06  
Purpose: Convert the broad portal plan into a small, executable sequence with tight scope.

## 1. Current Reality

- Backend transport and core API routes already exist (`/api/v3/runtime/*`, `/api/v3/settings`, `/api/v3/cards`, `/api/v3/command`).
- Frontend pages exist in `data/` (`index.html`, `config.html`, `settings.html`, `learn.html`) and are directly served by firmware.
- Main risk now is not architecture; it is scope sprawl and too many parallel tasks.

## 2. Rules To Reduce Overload

- Keep max 1 in-progress portal slice at a time.
- Every slice must be demoable on device, not only in code.
- No new feature starts until current slice has:
- API contract check
- mobile smoke check
- one failure-path check
- If a task takes more than 2 days, split it into two smaller deliverables.

## 3. Next 4 Slices (Do In Order)

## Slice P1: Runtime Page Stability Pass

Goal:
- Make Live page robust under disconnect/reconnect and stale-data windows.

Scope:
- Harden `/api/v3/runtime/cards/delta` consumption on frontend.
- Add explicit UI state badges: `ONLINE`, `RECONNECTING`, `OFFLINE`, `STALE`.
- Add retry backoff visualization.

Done when:
- Unplug/replug WiFi test shows no frozen UI.
- Card list resync works after missed sequences.
- No ambiguous "looks live but stale" state.

## Slice P2: Settings Reliability Pass

Goal:
- Make settings flow predictable and auditable.

Scope:
- Tighten request/response handling for `GET/PUT /api/v3/settings`.
- Show field-level validation feedback in UI.
- Add explicit `requiresRestart` handling message and action hint.

Done when:
- Invalid payload gives user-visible reason.
- Successful save always shows active revision and restart requirement.
- Mobile form interaction stays usable with keyboard open.

## Slice P3: Config Studio Thin Vertical Slice

Goal:
- Ship minimal but real config editing path first.

Scope:
- Build cards index panel from `GET /api/v3/cards`.
- Build single-card edit/load/save loop via `GET/PUT /api/v3/cards/{id}`.
- Start with DI + AI edit support only.

Done when:
- Operator can edit one DI card and one AI card end-to-end.
- Validation errors round-trip to UI.
- Commit failure path is handled with clear message.

## Slice P4: Command UX + Diagnostics Coupling

Goal:
- Make run-mode/step/force actions traceable from click to kernel apply.

Scope:
- Wire command actions to `/api/v3/command` with visible request IDs.
- Show latest command result + reason in runtime panel.
- Surface selected diagnostics counters for command parity checks.

Done when:
- Operator can trigger `setRunMode` and `stepOnce` from UI.
- Request ID from submit is visible in UI and matched in diagnostics data.
- Queue-full and reject reasons are visible without logs.

## 4. Minimal Weekly Cadence

- Monday: lock one slice scope and acceptance checks.
- Tuesday-Wednesday: implement only that slice.
- Thursday: hardware validation + bug fixes.
- Friday: update milestone/worklog and freeze.

## 5. Immediate Start (Today)

1. Start `P1` only.
2. Create a short test script for reconnect scenarios (manual steps acceptable).
3. Do not touch Config Studio this week.
