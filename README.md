# Advanced Timer V4 (ESP32-S3 Rebuild)

Status: Rebuild branch active (`esp32-s3-v4-rebuild`)

This branch is the clean-slate V4 implementation track focused on ESP32-S3.

Primary direction:
- deterministic runtime behavior
- strong memory management discipline
- low thermal footprint
- snappy portal behavior
- contract-first implementation

## Source Of Truth (Current)

- Migration/rebuild execution plan: `docs/esp32-s3-psram-migration-guide.md`
- V4 contract anchor (draft): `requirements-v4-contract.md`

## Legacy Reference

- Previous root overview preserved as: `legacy/README-legacy-v3.md`
- Existing V3 contracts/docs are retained as reference until replaced by V4 equivalents.

## Immediate Next

1. Define V4 contracts for platform, storage, runtime, and transport.
2. Stand up minimal ESP32-S3 boot skeleton.
3. Add telemetry baseline (CPU/core load, SRAM, PSRAM, heap, stack high-water).

