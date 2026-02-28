# Documentation Index

Date: 2026-02-28
Purpose: Canonical map of documentation for clean V3 transition.

## 1. Source Of Truth

- V3 production rewrite contract: `requirements-v3-contract.md` (root)
- V2 PoC frozen legacy contract: `README.md` (root)

## 2. Active V3 Docs (`docs/`)

- `docs/schema-v3.md` - Config schema and validation rules.
- `docs/api-contract-v3.md` - Transport/API envelope contract.
- `docs/acceptance-matrix-v3.md` - Acceptance test mapping.
- `docs/hardware-profile-v3.md` - Hardware profile/capability model.
- `docs/timing-budget-v3.md` - Runtime timing budget and threshold contract.
- `docs/fault-policy-v3.md` - Fault class/severity and safe-state policy contract.
- `docs/dependency-topology-rules.md` - Module dependency boundaries and layering rules.
- `docs/poc-gap-log-v3.md` - PoC-to-V3 gap traceability.
- `docs/decisions.md` - Decision log (accepted/superseded changes).
- `docs/worklog.md` - Consolidated session worklog.

Worklog rule:

- `docs/worklog.md` is the only canonical worklog file.
- Do not create date-suffixed worklog files (for example `worklog-YYYY-MM-DD.md`).

## 3. Legacy Planning Archive (`docs/legacy/`)

These files are retained for historical/planning context and are not the active production contract:

- `docs/legacy/firmware-rewrite-foundations.md`
- `docs/legacy/production-firmware-kickoff-plan.md`
- `docs/legacy/plc-problem-statements.md`

## 4. Root Folder Policy

- Keep only canonical anchors at repo root: `README.md`, `requirements-v3-contract.md`, and tool-specific root docs (for example `GEMINI.md`).
- Place all evolving design/spec/test docs under `docs/`.
- Place historical/planning artifacts under `docs/legacy/`.


