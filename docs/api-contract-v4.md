# API Contract V4 (Baseline)

## Scope

- Transport adapter over core services.
- Backend logic remains in core service modules.
- Initial endpoint set is intentionally minimal for frontend parallel work.

## Endpoints

### `GET /api/v4/status`

- Purpose: Return current runtime health snapshot.
- Response: `200 application/json`

```json
{
  "uptimeMs": 12345,
  "wifi": {
    "status": 3,
    "rssi": -52,
    "ip": "192.168.0.235"
  },
  "core0": {
    "avgUs": 2,
    "maxUs": 167,
    "overruns": 0,
    "loadPct": 0.02
  },
  "core1": {
    "avgUs": 121,
    "maxUs": 57714,
    "overruns": 1,
    "loadPct": 0.24
  },
  "memory": {
    "heapFree": 289332,
    "heapMin": 288236,
    "psramPresent": true,
    "psramFree": 8370855,
    "psramMin": 7337443
  },
  "stack": {
    "core0Words": 3392,
    "core1Words": 3804
  }
}
```

### Errors

- Unknown route: `404 application/json`

```json
{ "error": "not_found" }
```

## Notes

- Wi-Fi `status` uses Arduino `wl_status_t` numeric values.
- This baseline contract is additive-only for V4. Existing keys must stay stable.
