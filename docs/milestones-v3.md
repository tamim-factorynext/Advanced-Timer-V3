# V3 Milestones

Date: 2026-03-02  
Status: Active checkpoint tracker for V3 execution progress.

## 1. Usage Rules

- Update this file when a milestone starts, completes, is blocked, or changes scope.
- Keep milestone records concise and evidence-driven.
- Link each milestone to decision IDs and key files.
- Record the commit short SHA for the checkpoint update.

Status vocabulary:

- `PLANNED`
- `IN_PROGRESS`
- `DONE`
- `BLOCKED`

## 2. Milestone Ledger

## M0: Branch Safety Baseline
- Status: `DONE`
- Date: 2026-03-02
- Summary: froze rollback-safe baseline before fast-track rewrite.
- Outputs:
  - branch `legacy-stable`
  - branch `v3-core`
  - tag `v2-legacy-baseline-2026-03-02`
- References:
  - `DEC-0020`
- Checkpoint SHA: `d3b1840`

## M1: Composition-Root Architecture Baseline
- Status: `DONE`
- Date: 2026-03-02
- Summary: replaced monolithic entrypoint with service-boundary composition root.
- Outputs:
  - `src/main.cpp`
  - `src/kernel/kernel_service.*`
  - `src/runtime/runtime_service.*`
  - `src/control/control_service.*`
  - `src/storage/storage_service.*`
  - `src/portal/portal_service.*`
  - `src/platform/platform_service.*`
- References:
  - `DEC-0021`
  - `requirements-v3-contract.md` (architecture boundaries)
- Checkpoint SHA: `d3b1840`

## M2: Typed Config Contract + Validation Boot Gate
- Status: `DONE`
- Date: 2026-03-02
- Summary: added typed config contract, validator, and kernel boot through validated config only.
- Outputs:
  - `src/storage/v3_config_contract.*`
  - `src/storage/v3_config_validator.*`
  - `src/storage/storage_service.*`
  - `src/kernel/kernel_service.*`
  - `src/main.cpp` (fail-fast on invalid config)
- References:
  - `DEC-0021`
  - `requirements-v3-contract.md` (validation and ownership rules)
- Evidence:
  - firmware build `esp32doit-devkit-v1`: `SUCCESS` (2026-03-02)
- Checkpoint SHA: `d3b1840`

## M3: JSON Decode -> Typed SystemConfig
- Status: `PLANNED`
- Date: 2026-03-02
- Summary: parse staged payloads into `SystemConfig` and route through validator with explicit error mapping.
- Planned outputs:
  - decoder module in `src/storage/`
  - validation/error response mapping contract
  - tests for malformed payload and boundary values
- References:
  - `docs/schema-v3.md`
  - `docs/api-contract-v3.md`

## M4: Kernel Runtime Binding From Typed Config
- Status: `PLANNED`
- Date: 2026-03-02
- Summary: initialize runtime families from validated typed config and remove legacy-path dependencies from boot path.
- Planned outputs:
  - typed family runtime init binding
  - deterministic startup invariants
  - acceptance checks for config-to-runtime consistency
