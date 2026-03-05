# Decisions (V4)

Date: 2026-03-05
Branch: esp32-s3-v4-rebuild
Status: Active

## DEC-0001: Health Telemetry First

- Date: 2026-03-05
- Status: Accepted

### Context

V4 development has started on ESP32-S3 hardware. Before feature growth, we need ongoing visibility into runtime/resource health so regressions are visible early.

### Decision

Implement and maintain a first-class health telemetry stream during V4 implementation that tracks:

- Program storage (flash usage)
- LittleFS usage
- RAM/heap usage
- PSRAM usage
- Stack high-water/usage (core/task context)
- CPU usage/load per core

These metrics will be logged continuously as development proceeds.

### Implications

- Add a dedicated V4 health-monitor module early in the codebase.
- Keep telemetry sampling lightweight and interval-based.
- Treat telemetry output as a required development signal, not optional debug noise.
- Use collected data to guide performance/stability decisions throughout implementation.

## DEC-0002: Split Config Artifacts with Atomic Pair Activation

- Date: 2026-03-05
- Status: Accepted

### Context

V4 needs cleaner separation than V3 between system-level settings and card runtime logic configuration.

### Decision

Persist configuration in two separate LittleFS JSON artifacts:

- `settings`
- `card_config`

Activation/commit must still be atomic at pair level. Mixed or incompatible revisions are invalid.

### Implications

- Validation must run on both artifacts and cross-artifact references together.
- Boot/recovery must use compatible artifact pairs.
- Metadata (`schemaVersion`, `commitId`, compatibility markers) is required for safe pair activation.

## DEC-0003: LKG Internal-Only and UI Action Placement

- Date: 2026-03-05
- Status: Accepted

### Context

User-facing rollback to LKG can expose stale/opaque states and complicate safe UX.

### Decision

- Keep LKG as internal robustness mechanism only (automatic recovery path).
- Do not expose user/API restore-to-LKG action.
- User-facing restore options:
  - factory reset
  - import validated config
- Portal placement:
  - `Config` page owns backup/restore (`export`, `import`, `import validation`, apply/commit)
  - `Settings` page owns device-level reset/status operations (including factory reset and recovery status visibility)

### Implications

- Runtime/boot may auto-recover to LKG when integrity checks fail.
- Recovery source/events must be visible in diagnostics/logs.
- User workflows stay clear: config lifecycle in Config page, device/system actions in Settings page.
