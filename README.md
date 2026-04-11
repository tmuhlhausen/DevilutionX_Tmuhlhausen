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

# What this fork adds

This fork extends DevilutionX with multiplayer-focused systems for raids, guild progression, rollback-aware synchronization, and network telemetry tooling for debugging and live iteration.

- **Raid gameplay loop**: Party raid state, sync, and progression logic designed for repeatable group encounters.
- **Guild progression**: Shared progression structures that track account/group advancement over time.
- **Rollback state management**: Deterministic multiplayer recovery/state rewind scaffolding for desync resilience.
- **Net telemetry**: Instrumentation and traces for network behavior, rollback diagnostics, and protocol tuning.

# Repo map

- `Source/raid` - Raid state, raid progression, protocol helpers, and raid runtime logic.
- `Source/guild` - Guild progression models and related progression handling.
- `Source/dvlnet` - Multiplayer transport, rollback state, and telemetry integration points.
- `test` - Unit/integration tests, including raid, rollback, and telemetry coverage.
- `docs` - Contributor, build, and project documentation.
- `CMake` - Build configuration modules and platform/toolchain setup.
- `Packaging` - Packaging scripts and assets for release targets.

# Multiplayer, raids, and progression (fork focus)

Core fork-specific implementation files include:

- `Source/raid/raid_state.hpp` and `Source/raid/raid_state.cpp`
- `Source/raid/raid_progression.hpp` and `Source/raid/raid_progression.cpp`
- `Source/guild/guild_progression.hpp` and `Source/guild/guild_progression.cpp`
- `Source/dvlnet/rollback_state.hpp` and `Source/dvlnet/rollback_state.cpp`
- `Source/dvlnet/net_telemetry.hpp` and `Source/dvlnet/net_telemetry.cpp`

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
