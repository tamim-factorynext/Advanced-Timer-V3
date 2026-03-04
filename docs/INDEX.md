# Documentation Index

Date: 2026-03-04
Purpose: Canonical map of active documentation on the V4 rebuild branch.

## 1. Source Of Truth (Branch: `esp32-s3-v4-rebuild`)

- V4 rebuild contract anchor: `requirements-v4-contract.md` (root, draft)
- Project-level introduction/orientation: `README.md` (root, V4)
- Rebuild execution plan: `docs/esp32-s3-psram-migration-guide.md`
- V2 PoC frozen legacy contract: `docs/legacy/v2-poc-contract.md`
- V3 prior root overview: `README-legacy-v3.md`

## 2. Active Docs (`docs/`)

Documentation creation policy on this branch:

- Keep existing V3 documents as legacy/reference unless and until a V4 replacement is actually needed by implementation work.
- Do not mass-copy or mass-rename V3 docs into V4 docs.
- Create new V4 `.md` files only when there is an active coding task that requires that contract/spec.

- `docs/schema-legacy-v3.md` - V3 schema reference (legacy baseline until V4 replacement exists).
- `docs/ARCHITECTURE.md` - V3 runtime architecture reference (legacy baseline).
- `docs/api-contract-legacy-v3.md` - V3 API contract reference (legacy baseline).
- `docs/acceptance-matrix-legacy-v3.md` - V3 acceptance mapping reference.
- `docs/hardware-profile-legacy-v3.md` - V3 hardware profile/capability reference.
- `docs/timing-budget-legacy-v3.md` - V3 timing budget reference.
- `docs/fault-policy-legacy-v3.md` - V3 fault policy reference.
- `docs/dependency-topology-rules.md` - Module dependency boundaries and layering rules.
- `docs/poc-gap-log-legacy-v3.md` - PoC-to-V3 gap traceability.
- `docs/decisions.md` - Decision log (accepted/superseded changes).
- `docs/worklog.md` - Consolidated session worklog.
- `docs/milestones-legacy-v3.md` - Historical V3 milestone ledger.
- `docs/hil-task-list-legacy-v3.md` - Historical V3 HIL checklist.
- `docs/user-guide-legacy-v3-draft.md` - Historical V3 user guide draft.
- `docs/esp32-s3-psram-migration-guide.md` - Active V4 rebuild and migration source of truth.
- `docs/naming-glossary-legacy-v3.md` - Canonical user-facing naming vocabulary and staged key-migration plan.
- `docs/doxygen-comment-style.md` - Standard Doxygen comment templates and rollout guidance for symbol-level documentation.

## 2.1 Market/Strategy Research

- `docs/market/market-fit-open-source-iot-firmware-2026.md` - Long-form external market landscape and positioning research input (Gemini deep research synthesis).

Worklog rule:

- `docs/worklog.md` is the only canonical worklog file.
- Do not create date-suffixed worklog files (for example `worklog-YYYY-MM-DD.md`).

## 3. Legacy Planning Archive (`docs/legacy/`)

These files are retained for historical/planning context and are not the active production contract:

- `docs/legacy/firmware-rewrite-foundations.md`
- `docs/legacy/production-firmware-kickoff-plan.md`
- `docs/legacy/plc-problem-statements.md`
- `docs/legacy/v2-poc-contract.md`

## 4. Root Folder Policy

- Keep canonical anchors at repo root: `README.md`, `requirements-v4-contract.md`, and tool-specific root docs (for example `GEMINI.md`).
- Place all evolving design/spec/test docs under `docs/`.
- Place historical/planning artifacts under `docs/legacy/`.



