# Raid + Guild Mod API (v1)

This document defines the **stable extension points** for raid and guild systems.

## Versioned interfaces

### Raid API
- Header: `Source/raid/raid_mod_api.hpp`
- Current version: `CurrentRaidModApiVersion = 1`
- Registration:
  - `RegisterRaidModHooks(moduleName, apiVersion, hooks)`
- Hooks:
  - `onRaidStateChanged(const RaidInstanceState&)`
  - `onRaidReset(const RaidInstanceState&)`
  - `onRaidCompleted(const RaidInstanceState&)`
  - `onRaidFailed(const RaidInstanceState&)`

### Guild API
- Header: `Source/guild/guild_mod_api.hpp`
- Current version: `CurrentGuildModApiVersion = 1`
- Registration:
  - `RegisterGuildModHooks(moduleName, apiVersion, hooks)`
- Hooks:
  - `onGuildHallChanged(const GuildHallState&)`
  - `onGuildMemberChanged(uint8_t playerId, const GuildMemberState&)`

## Runtime compatibility contract

Both registration APIs enforce strict version matching.

- If `apiVersion == Current*ModApiVersion`: hooks are activated.
- If `apiVersion != Current*ModApiVersion`: registration is rejected and built-in behavior remains active.

Use status APIs for diagnostics:
- `GetRaidModCompatibilityStatus()`
- `GetGuildModCompatibilityStatus()`

The compatibility struct reports:
- `compatible`
- `requestedVersion`
- `activeVersion`
- `moduleName`

## Data contracts

### Raid state contract
`RaidInstanceState` is passed by const reference and includes authoritative values such as:
- `raidId`, `difficulty`, `phase`, `result`, `lockout`
- membership and readiness bitmasks
- objective/checkpoint bits
- timers, lockout expiration
- `snapshotRevision`, `sequence`

### Guild state contract
- `GuildHallState` includes guild identity and aggregate membership counters.
- `GuildMemberState` includes guild binding, role, permission bitset, and invite flag.

## Hook timing guarantees

Raid:
- Reset: called after reset is applied.
- State changed: called after accepted snapshots and encounter progression updates.
- Completed/Failed: called after transition is committed.

Guild:
- Hall changed: called when guild hall aggregate state is updated/applied.
- Member changed: called after member state persistence updates.

## Constraints and safety

- Hooks execute synchronously on the game thread.
- Hooks must be non-blocking and deterministic.
- Do not mutate engine state from hook callbacks unless explicitly supported by a future API version.
- Compatibility mismatch does **not** abort gameplay; engine falls back to native behavior.

## Ops commands (chat)

- `/ops snapshot` — compact telemetry + compatibility snapshot.
- `/ops raiddiag` — raid instance diagnostics dump.
- `/ops repairprog` — progression repair action (clears raid checkpoints across difficulties).
