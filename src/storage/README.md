# Storage Layer Skeleton

Purpose: persistence and config lifecycle storage boundaries.

Planned scope:
- staged config file IO
- commit/rollback slot operations
- version metadata persistence

Current interfaces:
- `legacy_config_lifecycle.h`
- `legacy_v3_normalizer.h`
- `legacy_v3_config_service.h`

