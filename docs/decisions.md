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
