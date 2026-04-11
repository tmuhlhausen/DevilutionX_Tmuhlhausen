<p align="center">
  <img width="560" height="144" alt="image" src="https://github.com/user-attachments/assets/7ac73801-ef7b-4cc1-8442-a191a2a0a1ce" />
</p>

---

[![Discord Channel](https://img.shields.io/discord/518540764754608128?color=%237289DA&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/devilutionx)
[![Downloads](https://img.shields.io/github/downloads/diasurgical/devilutionX/total.svg)](https://github.com/diasurgical/devilutionX/releases/latest)
[![Codecov](https://codecov.io/gh/diasurgical/devilutionX/branch/master/graph/badge.svg)](https://codecov.io/gh/diasurgical/devilutionX)

<p align="center">
<img width="853" height="480" alt="image" src="https://github.com/user-attachments/assets/ee902926-6382-4ee5-b1c2-7947e8b434e9" />
</p>

<sub>*(The health-bar and XP-bar are off by default but can be enabled in the [game settings](https://github.com/diasurgical/DevilutionX/wiki/Config-File). Widescreen can also be disabled if preferred.)*</sub>

# What is DevilutionX

DevilutionX is a port of Diablo and Hellfire that strives to make it simple to run the game while providing engine improvements, bug fixes, and some optional quality of life features.

Check out the [manual](https://github.com/diasurgical/devilutionX/wiki) for available features and how to take advantage of them.

For a full list of changes, see our [changelog](docs/CHANGELOG.md).

## Roadmap

This roadmap tracks fork-specific features and the delivery targets needed to ship them safely.

**Approval flow for every roadmap item:** idea ➜ RFC issue ➜ prototype PR ➜ integration PR.

### Raid Content Pipeline ([docs/raid/](docs/raid/))
- Define a deterministic encounter script schema and ship `>= 20` validation tests across boss phases, loot tables, and fail states.
- Add a raid content compiler with lint gates that blocks merges on schema violations and keeps compile time under `2 minutes` for a full raid bundle.
- Deliver reusable encounter simulation tooling that can replay `1,000+` scripted runs with identical outcomes across two consecutive CI jobs.
- Publish a content authoring checklist and reference templates to reduce first-pass review comments by `30%` over two milestones.

### Guild Economy & Rewards ([docs/guild-economy/](docs/guild-economy/))
- Introduce guild treasury transaction types with invariant checks that keep ledger drift at `0` in nightly audit tests.
- Implement weekly reward distribution rules with automated tests covering `100%` of payout branches and edge cases (join/leave mid-cycle, ties, inactivity).
- Add anti-inflation sinks and balancing telemetry, targeting a monthly net currency delta within `±5%` of design forecast.
- Provide guild progression dashboards and event exports so admins can reconcile reward outcomes in under `10 minutes` per cycle.

### Netcode Reliability ([docs/netcode/](docs/netcode/))
- Add end-to-end deterministic state-sync tests for high-latency scenarios (`150ms`, `250ms`, `400ms`) with a desync rate target below `0.1%`.
- Implement packet loss recovery and resend backoff, reducing disconnects from simulated `5%` packet loss sessions by `>= 40%`.
- Ship host migration smoke coverage that completes reconnection in under `8 seconds` for `90%` of test sessions.
- Add protocol compatibility checks and wire-version docs to guarantee backward compatibility across one minor version.

### Observability & Ops ([docs/ops/](docs/ops/))
- Instrument structured gameplay/network logs with trace IDs, achieving `>= 95%` coverage on critical multiplayer paths.
- Build service-level dashboards (latency, disconnects, desyncs, crash-free sessions) with alert thresholds tied to release gates.
- Add on-demand diagnostics bundles that capture logs/config/runtime metadata and cut triage-to-root-cause time by `50%`.
- Document incident response runbooks and on-call escalation paths with quarterly drill completion tracked in `docs/ops/`.

### Modding API ([docs/modding-api/](docs/modding-api/))
- Publish a versioned API surface map and semantic compatibility policy, with CI checks blocking undocumented public API changes.
- Deliver sandboxed script hooks for combat, loot, and events, including performance budgets (`< 1ms` average hook overhead per tick).
- Add an integration test suite of `25+` canonical mods to validate API behavior across patch upgrades.
- Provide starter mod templates and packaging docs that reduce new-mod setup time to under `15 minutes` for first-time contributors.


# How to Install

You must provide original game data to run this project:

- **Required for full Diablo**: `DIABDAT.MPQ`.
- **Shareware-only alternative**: `spawn.mpq` can be used *instead of* `DIABDAT.MPQ`, but only enables the shareware portion.
- **Required for Hellfire expansion**: `hellfire.mpq`, `hfmonk.mpq`, `hfmusic.mpq`, and `hfvoice.mpq`.

If you do not already own the game data, you can obtain Diablo from [GoG.com](https://www.gog.com/game/diablo) or Battle.net. You can also [extract `DIABDAT.MPQ` from the GoG installer](https://github.com/diasurgical/devilutionX/wiki/Extracting-MPQs-from-the-GoG-installer). The shareware `spawn.mpq` is available from [devilutionx-assets](https://github.com/diasurgical/devilutionx-assets/releases/latest/download/spawn.mpq) and Blizzard's [historic demo mirror](http://ftp.blizzard.com/pub/demos/diablosw.exe).

Download the latest [DevilutionX release](https://github.com/diasurgical/devilutionX/releases/latest), extract it, then place the required MPQ files next to the executable. Alternatively, [build from source](#building-from-source).

For full platform-specific steps, see [Installation Instructions](docs/installing.md).

# Contributing

We are always looking for more people to help with [coding](docs/CONTRIBUTING.md), [documentation](https://github.com/diasurgical/devilutionX/wiki), [testing the latest builds](#test-builds), spreading the word, or hanging out on our [Discord server](https://discord.gg/devilutionx).

Contributor quick pointers:

- Start with the contributor guide: [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).
- Raid-focused tests: `test/raid_state_test.cpp`, `test/raid_progression_test.cpp`, `test/raid_sync_test.cpp`, `test/raid_protocol_test.cpp`.
- Rollback-focused test: `test/rollback_state_test.cpp`.

# Mods

We hope to provide a good starting point for mods. In addition to the full Devilution source code, we also provide modding tools. Check out the list of known [mods based on DevilutionX](https://github.com/diasurgical/devilutionX/wiki/Mods).

# Test Builds

If you want to help test the latest development version (make sure to back up your files, as these may contain bugs), you can fetch the test build artifact from one of the build servers:

*Note: You must be logged into GitHub to download the attachments!*

[![Linux x86_64](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64.yml?query=branch%3Amaster)
[![Linux AArch64](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_aarch64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_aarch64.yml?query=branch%3Amaster)
[![Linux x86](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86.yml?query=branch%3Amaster)
[![Linux x86_64 SDL1](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64_SDL1.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64_SDL1.yml?query=branch%3Amaster)
[![macOS x86_64](https://github.com/diasurgical/devilutionX/actions/workflows/macOS_x86_64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/macOS_x86_64.yml?query=branch%3Amaster)
[![Windows MinGW x64](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x64.yml?query=branch%3Amaster)
[![Windows MinGW x86](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x86.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x86.yml?query=branch%3Amaster)
[![Windows MSVC x64](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MSVC_x64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MSVC_x64.yml?query=branch%3Amaster)
[![Android](https://github.com/diasurgical/devilutionX/actions/workflows/Android.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Android.yml?query=branch%3Amaster)
[![iOS](https://github.com/diasurgical/devilutionX/actions/workflows/iOS.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/iOS.yml?query=branch%3Amaster)
[![PS4](https://github.com/diasurgical/devilutionX/actions/workflows/PS4.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/PS4.yml?query=branch%3Amaster)
[![Original Xbox](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_nxdk.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_nxdk.yml?query=branch%3Amaster)
[![Xbox One/Series](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_one.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_one.yml?query=branch%3Amaster)
[![Nintendo Switch](https://github.com/diasurgical/devilutionX/actions/workflows/switch.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/switch.yml)
[![Sony PlayStation Vita](https://github.com/diasurgical/devilutionX/actions/workflows/vita.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/vita.yml)
[![Nintendo 3DS](https://github.com/diasurgical/devilutionX/actions/workflows/3ds.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/3ds.yml)
[![Amiga M68K](https://github.com/diasurgical/devilutionX/actions/workflows/amiga-m68k.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/amiga-m68k.yml)

# Building from Source

Want to compile the program by yourself? Great! Simply follow the [build instructions](docs/building.md).

# Credits

- The original Devilution project: [Devilution](https://github.com/diasurgical/devilution#credits)
- [Everyone](https://github.com/diasurgical/devilutionX/graphs/contributors) who worked on Devilution/DevilutionX
- [Nikolay Popov](https://www.instagram.com/nikolaypopovz/) for UI and graphics
- [WiAParker](https://wiaparker.pl/projekty/diablo-hellfire/) for the Polish voice pack
- And thanks to all who support the project, report bugs, and help spread the word ❤️

# Legal

DevilutionX is made publicly available and released under the Sustainable Use License (see [LICENSE](LICENSE.md)).

The source code in this repository is for non-commercial use only. If you use the source code, you may not charge others for access to it or any derivative work thereof.

Diablo® - Copyright © 1996 Blizzard Entertainment, Inc. All rights reserved. Diablo and Blizzard Entertainment are trademarks or registered trademarks of Blizzard Entertainment, Inc. in the U.S. and/or other countries.

DevilutionX and any of its maintainers are in no way associated with or endorsed by Blizzard Entertainment®.
