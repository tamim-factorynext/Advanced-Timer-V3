# Storage Layer Skeleton

Purpose: persistence and config lifecycle storage boundaries.

Planned scope:
- staged config file IO
- commit/rollback slot operations
- version metadata persistence

Current interfaces:
- `config_lifecycle.h`
- `v3_normalizer.h`
- `v3_config_service.h`
