# Resource Metrics Log (V4)

Purpose: Track runtime resource usage in project history (not in LittleFS) as V4 features are added.

## Capture Format

- Date
- Build/profile (`board`, CPU frequency)
- Scenario (offline/connected/test flow)
- Key metrics snapshot from serial:
  - Core0 avg/max/overrun and load
  - Core1 avg/max/overrun and load
  - Heap free/min
  - PSRAM present/free/min
  - Stack high-water C0/C1
- Notes (regressions, thermal behavior, Wi-Fi stability)

## Entries

### 2026-03-05 - Baseline (cool-mode Wi-Fi policy, 160 MHz)

- Profile: `esp32-s3-r8n16-v4`, `board_build.f_cpu=160000000L`
- Scenario: Boot + Wi-Fi retry/connect observations
- Snapshot sample:
  - `C0(avg/max/ovr): 2/167/0 us`
  - `C1(avg/max/ovr): 121/57714/1 us`
  - `heap free/min: 289332/288236`
  - `psram present: yes free/min: 8370855/7337443`
  - `stackHW words C0/C1: 3392/3804`
- Notes:
  - Board showed unstable Wi-Fi behavior across runs; cool-mode retry policy applied.
  - Thermal suspicion remains open until validation on replacement module.
