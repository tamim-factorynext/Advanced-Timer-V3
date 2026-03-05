# V4 Worklog

Date: 2026-03-04
Branch: esp32-s3-v4-rebuild
Status: Active

## Session Summary

- Created and switched to dedicated V4 branch: `esp32-s3-v4-rebuild`.
- Reworked migration strategy docs around clean `v4` rebuild direction.
- Added rebuild watchlist and telemetry tracking expectations to migration plan.
- Completed mass rename/isolation pass of prior V3 artifacts to legacy naming.
- Moved prior implementation assets out of active paths:
  - `legacy/src_legacy_v3/`
  - `legacy/data_legacy_v3/`
  - `legacy/docs_legacy_v3/`
- Preserved prior runtime entrypoint in `legacy/main_legacy_v3.cpp`.
- Replaced active firmware entry with minimal V4 bootstrap in `src/main.cpp`.
- Moved root legacy anchors into `legacy/`:
  - `legacy/README-legacy-v3.md`
  - `legacy/requirements-legacy-v3-contract.md`
  - `legacy/GEMINI.md`
- Updated active docs index to reflect V4-first + legacy archive layout.
- Added V4 core contract doc (`docs/core-contract-v4.md`) by importing/adapting V3 dual-core and Wi-Fi contracts.
- Added explicit V4 alignment check for dual-core determinism, connectivity resilience, and telemetry-first requirements.
- Expanded `docs/core-contract-v4.md` with card-family contract sections imported from V3 and adapted for V4.
- Added shared card model, set/reset semantics, and per-family baseline requirements for `DI`, `AI`, `DO`, `SIO`, `MATH`, and `RTC`.
- Added explicit card sub-parameter details, units/value conventions, and runtime behavior clauses in `docs/core-contract-v4.md`.
- Added explicit validation rule that `AI` and `RTC` must not carry/evaluate set/reset blocks.
- Added explicit per-card set/reset impact matrix and missing/unsupported set/reset validation rules to remove ambiguity.
- Added explicit force/mask behavior contract with precedence, per-card applicability, and validation/diagnostics rules.
- Added missing numeric contract clauses in V4 core contract: fixed-point centiunit representation and non-negative value enforcement.
- Added second-pass card contract completeness items: condition block structure/combiners, compare-source/state constraints, and card variable binding/topology ownership rules.
- Added third-pass card semantics: safe inert set/reset default guidance, DI edgePulse one-scan rule, DO/SIO zero-timer behavior, and RTC retrigger-policy requirement.
- Ran cross-document legacy sweep and tightened V4 contract modality (`MUST` vs ambiguous terms) for key validation/runtime clauses.
- Added explicit runtime signal projection mapping by family (`DI/DO/SIO`, `AI`, `RTC`) to close shared-snapshot semantics gap.
- Added global numeric ceiling policy: `0..100,000,000` centiunits (`1,000,000.00` display max) for AI/MATH/counter `liveValue` paths, including saturation-at-ceiling behavior.
- Aligned user guide counter and numeric-limit wording with the new V4 ceiling policy.
- Rewrote `docs/core-contract-v4.md` into a coherent single-flow V4 contract with consolidated clauses (architecture, numeric model, cards, set/reset, force/mask, topology, Wi-Fi, observability).
- Added explicit alignment verdict and next improvement targets inside the core contract.
- Expanded `README.md` with project identity, core intent, target users, why/market gap rationale, and competition-positioning snapshot.
- Extended README identity documentation with additional legacy-derived context: explicit goals/non-goals, behavior promise, stakeholder profile, and operating principles.
- Updated `platformio.ini` with documented ESP32-S3 V4 GPIO profile defaults (UART/I2C/SPI plus initial DI/DO/AI logical pin lists) using compile-time build flags.
- Updated `docs/core-contract-v4.md` with explicit hardware profile/backend-selection contract (GPIO/I2C expander/plugin paths + family capacities).
- Added V4 persistence contract for split LittleFS artifacts (`settings` and `card_config`) including atomic commit, compatibility metadata, and LKG pair fallback rules.
- Refined restore policy: LKG kept as internal robustness-only fallback; user-facing rollback to LKG is explicitly disallowed (factory/import only).
- Added decisions `DEC-0002` (split artifacts with atomic pair activation) and `DEC-0003` (LKG internal-only + Config/Settings action placement).
- Rewrote related persistence/restore contract sections for coherent flow and explicit portal page ownership of backup/restore vs settings actions.
- Imported legacy user guide into active V4 docs as `docs/user-guide-v4.md` with targeted alignment edits.
- Updated Wi-Fi user-guide behavior to match V4 core contract (backup-first, user fallback, offline mode, background retry).

## Verification

- PlatformIO build succeeds with minimal V4 bootstrap (`src/main.cpp`).
- Active root now contains only V4 anchors and build config.

## Next Session Start Point

1. Define V4 module contracts (platform, storage, runtime, transport).
2. Create initial ESP32-S3 environment in `platformio.ini`.
3. Add baseline telemetry scaffolding (CPU/core load, SRAM/PSRAM/heap, stack high-water).

## Deferred Notes (Contract Hardening Backlog)

1. Add acceptance-test IDs per major core-contract clause (`AT-*` / `HIL-*`) for traceability.
2. Split transport/API and config lifecycle into dedicated V4 contract files and cross-link from core contract.
3. Add a single per-field min/max appendix for all card config parameters to remove parser ambiguity.
