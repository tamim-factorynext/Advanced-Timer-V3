# ESP32-S3 PSRAM Migration Guide (Planned)

Date: 2026-03-03  
Status: Planned (deferred until ESP32-S3 hardware is available)

## 1. Purpose

Capture a concrete migration path from current ESP32 target to ESP32-S3 + PSRAM so memory-pressure fixes are intentional, testable, and reversible.

## 2. Why This Exists

On 2026-03-02, firmware hit an ESP32 linker DRAM overflow (`dram0_0_seg`) after runtime snapshot/diagnostics payload growth.

Root cause:

- Large fixed portal JSON buffers were allocated in static memory (`.bss`) via `char[]`.
- Static data growth reduced internal SRAM headroom enough to fail linking.

Immediate mitigation already applied:

- Replaced fixed `char[]` buffers with heap-backed `String` in `PortalService`.
- Build recovered, but long-term memory policy remains important.

Current guardrails (added 2026-03-03):

- Pre-reserve portal JSON `String` capacities at startup to reduce realloc churn.
- Reject oversized JSON builds when `measureJson(...)` exceeds reserved capacity.
- Return explicit fallback JSON payloads on memory pressure instead of partial payloads.
- Expose heap-guard counters in diagnostics payload:
  - reserve readiness
  - serialize-failure counts
  - capacity-reject counts
  - current string capacities

Recent bring-up pain points (observed on 2026-03-04, DOIT ESP32 DevKit V1):

- LittleFS mount corruption occurred on boot and required auto-format recovery path (`LittleFS.begin(true)` fallback).
- Multiple stack overflow/canary crashes occurred in `core0`/`core1` tasks during early runtime until stack-hardening changes were applied.
- Memory headroom remains sensitive when snapshot/diagnostics/transport paths grow.
- Current LittleFS ceiling on this target remains limited (`~1.4 MB`) for firmware + portal asset growth planning.

## 3. What `.bss` Means (Memory Model Note)

`.bss` is the binary section for global/static variables that are zero-initialized at startup.

Practical implications:

- Any large global/static arrays consume internal SRAM permanently for the full runtime.
- This memory is reserved before tasks run; it does not scale down when idle.
- If `.bss` + `.data` + runtime overhead exceed target DRAM limits, link fails before flashing.

## 4. PSRAM Suitability Rules

Use PSRAM for large, non-real-time, bursty, or transport-facing buffers.

Good candidates:

- Snapshot/diagnostics JSON assembly buffers
- Portal/web response staging buffers
- Optional telemetry history rings used only for UI/diagnostics
- Large temporary parse/format work buffers

Keep in internal SRAM:

- Deterministic scan-loop hot state
- ISR-shared data and tight synchronization objects
- Frequently accessed kernel runtime structures
- Latency-sensitive queue control metadata

## 5. Migration Objectives

- Make lower thermal output a primary migration goal (reduce sustained board heat during normal runtime).
- Increase memory headroom for portal observability payload growth.
- Avoid reintroducing large static `.bss` buffers.
- Preserve deterministic kernel timing behavior.
- Keep rollback path to current ESP32 target until S3 path is validated.

## 6. Planned Migration Steps

1. Add a new PlatformIO environment for ESP32-S3 board variant with PSRAM enabled.
2. Keep existing ESP32 environment untouched as fallback.
3. Introduce a small allocation policy layer for large portal buffers:
   - prefer PSRAM allocation on S3 targets
   - fall back safely to internal heap if PSRAM unavailable
4. Migrate portal snapshot/diagnostics staging away from implicit `String` growth into explicit bounded buffers allocated by policy.
5. Optionally stream JSON directly to transport where feasible to reduce peak buffering.
6. Add runtime memory telemetry endpoints/counters:
   - free internal heap
   - largest internal block
   - free PSRAM
   - largest PSRAM block
7. Run stress soak with sustained snapshot publish rates and verify:
   - no watchdog resets
   - no allocator failure
   - stable scan timing envelopes
   - no abnormal sustained board heating versus current ESP32 baseline under equivalent workload
8. Make S3+PSRAM target default only after acceptance evidence is recorded.

## 7. Acceptance Checklist For S3 Migration

- Build/link succeeds with PSRAM-enabled S3 environment.
- Snapshot API and portal payload contracts remain unchanged.
- Deterministic scan timing budget remains within existing thresholds.
- No regression in command/snapshot queue behavior.
- 24h soak test without heap fragmentation-related instability.
- Documented fallback behavior when PSRAM allocation fails.
- Thermal behavior is validated as a first-class criterion:
  - board remains within acceptable touch temperature during 30-minute normal-run baseline
  - no sustained thermal climb trend under equivalent Wi-Fi + snapshot traffic compared to current ESP32 baseline

## 8. Risks And Mitigations

Risk: Treating PSRAM as a universal memory pool may hurt deterministic paths.  
Mitigation: Strict rule that kernel hot state remains internal SRAM.

Risk: Fragmentation from variable-sized `String` growth under continuous updates.  
Mitigation: Prefer bounded reusable buffers for large payload assembly.

Risk: Runtime failure is harder to triage than build-time `.bss` overflow.  
Mitigation: Keep explicit fallback responses and publish memory guard counters for field diagnostics.

Risk: Board-to-board PSRAM differences (2MB/8MB, speed, config).  
Mitigation: Detect capacity/features at runtime and keep conservative defaults.

Risk: S3 migration could increase feature scope and accidentally keep CPU/wifi duty high, negating thermal gains.  
Mitigation: Track thermal impact as a gated acceptance item and prefer runtime-efficiency changes (reduced busy-loop churn, controlled publish rates, and conservative radio activity) during migration.

## 9. Migration Trigger Criteria

Start ESP32-S3 migration immediately when any of these persist after one stabilization pass:

- Repeated task stack overflows/canary triggers after normal stack tuning.
- Inability to keep runtime + portal payload features without memory-related resets/degradation.
- Portal asset/config growth pressure that cannot be kept safely inside current ESP32 LittleFS budget.
- Ongoing need to trade away diagnostics/usability features only to stay within SRAM/flash limits.

## 10. References

- `docs/decisions.md` (`DEC-0051`, `DEC-0052`)
- `docs/worklog.md` (2026-03-02 DRAM overflow + recovery entry)
- `src/portal/portal_service.h`
- `src/portal/portal_service.cpp`
