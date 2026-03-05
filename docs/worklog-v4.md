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
- Imported legacy user guide into active V4 docs as `docs/user-guide-v4.md` with targeted alignment edits.
- Updated Wi-Fi user-guide behavior to match V4 core contract (backup-first, user fallback, offline mode, background retry).

## Verification

- PlatformIO build succeeds with minimal V4 bootstrap (`src/main.cpp`).
- Active root now contains only V4 anchors and build config.

## Next Session Start Point

1. Define V4 module contracts (platform, storage, runtime, transport).
2. Create initial ESP32-S3 environment in `platformio.ini`.
3. Add baseline telemetry scaffolding (CPU/core load, SRAM/PSRAM/heap, stack high-water).
