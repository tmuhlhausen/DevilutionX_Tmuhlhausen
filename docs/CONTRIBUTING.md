# DevilutionX Contribution Guide
Welcome! Please review our [Contribution Guide](https://github.com/diasurgical/DevilutionX/wiki/Contributing) for more information!

_“A project is only as strong as its contributors. Thank you for helping us keep Diablo 1 alive and better than ever!”_

## 10-Minute Developer Onboarding
1. Clone the repository and open it in your preferred editor.
2. Read `docs/building.md` to get your local build/test flow ready.
3. Inspect `Source/raid/raid_state.cpp` to learn the current raid state flow.
4. Inspect `Source/dvlnet/rollback_state.hpp` to understand rollback synchronization data.
5. Inspect `test/raid_sync_test.cpp` to see how raid/network behavior is validated.

### First good tasks
- **Docs sync:** Keep `docs/building.md` and code comments aligned when build/test flags change.
- **Telemetry assertions:** Add focused assertions to verify raid/network state metrics in existing tests.
- **Raid rule edge cases:** Add tests for unusual transitions (disconnect/reconnect, rollback boundaries, mixed party states).

### Naming conventions
Keep new identifiers aligned with existing module naming patterns to preserve discoverability and consistency:
- `raid_*` for raid systems and state transitions
- `guild_*` for guild progression and guild-scoped logic
- `net_*` for networking transport/synchronization paths

> [!CAUTION]
> Any gameplay-affecting change should ship with test coverage in `test/` (new tests or explicit updates to existing ones).
