# Platform Layer Skeleton

Purpose: hardware/RTOS integration boundaries.

Planned scope:
- pin/profile binding
- task/queue/watchdog adapters
- board-level service wrappers

Implemented profile flow:
- build flags in `platformio.ini` define active board profile
- `src/platform/hw_profile.*` materializes typed profile data
- `PlatformService` maps logical channels (`DI/DO/AI`) to backend channels
- kernel runtime uses logical `channel` values only; no direct pin constants
