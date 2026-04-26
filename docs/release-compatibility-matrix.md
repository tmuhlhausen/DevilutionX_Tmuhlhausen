# Release Compatibility Matrix

| Game Version | Raid API Version | Guild API Version | Migration Notes |
|---|---:|---:|---|
| 1.6.x (legacy, pre-extension-point) | N/A | N/A | No stable mod-facing hooks for raid/guild subsystems. |
| 1.7.0 | 1 | 1 | Introduces versioned registration APIs (`RegisterRaidModHooks`, `RegisterGuildModHooks`) and strict runtime compatibility checks with graceful fallback on mismatch. |
| 1.7.1+ | 1 (planned stable) | 1 (planned stable) | No migration required for v1 consumers; keep `apiVersion = 1`. |

## Migration checklist

From pre-1.7.0:
1. Integrate with versioned hook registrars.
2. Pass explicit API version during registration.
3. Handle registration failure by disabling optional module behavior.
4. Use `/ops snapshot` in multiplayer test sessions to validate compatibility status.

From 1.7.0 to later 1.7.x:
1. Keep v1 interfaces unchanged.
2. Re-test hook timing assumptions around raid reset/completion/failure.
