# Documentation Index

Date: 2026-03-04
Purpose: Canonical map of active documentation on the V4 rebuild branch.

## 1. Source Of Truth (Branch: `esp32-s3-v4-rebuild`)

- V4 rebuild contract anchor: `requirements-v4-contract.md` (root, draft)
- Project-level introduction/orientation: `README.md` (root, V4)
- Rebuild execution plan: `docs/esp32-s3-psram-migration-guide.md`
- V3 prior root overview: `README-legacy-v3.md`

## 2. Active Docs (`docs/`)

Documentation creation policy on this branch:

- Keep V3 documents as legacy/reference unless and until a V4 replacement is actually needed by implementation work.
- Do not mass-copy V3 docs into V4 docs.
- Create new V4 `.md` files only when there is an active coding task that requires that contract/spec.

- `docs/esp32-s3-psram-migration-guide.md` - Active V4 rebuild and migration source of truth.

## 3. Legacy Archive Location

Legacy V3 docs/assets/code were isolated out of active paths and moved under:

- `legacy/docs_legacy_v3/`
- `legacy/data_legacy_v3/`
- `legacy/src_legacy_v3/`

## 4. Root Folder Policy

- Keep canonical anchors at repo root: `README.md`, `requirements-v4-contract.md`, and tool-specific root docs (for example `GEMINI.md`).
- Place all evolving design/spec/test docs under `docs/`.
- Keep historical/planning/runtime artifacts under `legacy/`.



