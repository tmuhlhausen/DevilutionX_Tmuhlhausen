# Master Roadmap

This roadmap consolidates the repository audit findings into a single execution plan. It is intended to keep the fork shippable while the raid, guild, netcode, Lua modding, observability, and packaging work moves from scaffold to production-grade systems.

## Roadmap principles

- **Build first.** No feature work should be considered healthy until the default build and test matrix passes.
- **Runtime truth beats documentation promises.** Roadmap entries must clearly distinguish scaffolded docs from operational code.
- **Security is a release gate.** CI permissions, dependencies, networking, Lua mod loading, and release artifacts are part of the product surface.
- **Determinism is a design contract.** Raid, guild, rollback, and netcode systems must prove repeatable behavior under test.
- **Small verifiable slices.** Each item below has an acceptance gate so progress is measurable.

## Status legend

| Status | Meaning |
|---|---|
| `Blocked` | Known issue prevents safe release or reliable build. |
| `Planned` | Scope is defined, implementation not started or not proven. |
| `In Progress` | Implementation exists but still lacks required validation. |
| `Operational` | Feature is implemented, tested, documented, and release-gated. |

## P0: Build and link integrity

### P0.1 Add missing mod API implementation files to the build

**Finding:** `guild/guild.cpp` and `raid/raid_state.cpp` call functions implemented in `guild/guild_mod_api.cpp` and `raid/raid_mod_api.cpp`, but those implementation files are not listed in `Source/CMakeLists.txt`.

**Risk:** unresolved symbols, broken tests, or feature hooks silently unavailable depending on target/link behavior.

**Status:** Blocked

**Acceptance criteria:**
- `Source/CMakeLists.txt` includes `guild/guild_mod_api.cpp`.
- `Source/CMakeLists.txt` includes `raid/raid_mod_api.cpp`.
- Clean configure, build, and test pass with `BUILD_TESTING=ON` on at least Linux and Windows.
- Add a regression test that calls both guild and raid hook registration/reset paths.

**Validation commands:**

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j $(getconf _NPROCESSORS_ONLN)
ctest --test-dir build --output-on-failure
```

### P0.2 Quarantine or delete stale `Source/raid/raid.cpp`

**Finding:** `Source/raid/raid.cpp` contains an older parallel raid implementation with state and field names that diverge from the active `raid_state.cpp`/`raid_rules.cpp` model.

**Risk:** future work may modify the wrong implementation, or the stale file may be accidentally reintroduced into the build and cause duplicate or incompatible behavior.

**Status:** Planned

**Acceptance criteria:**
- Remove `Source/raid/raid.cpp`, or move it outside compileable source paths with an archival note.
- Confirm no CMake target references it.
- Add a short note in `docs/raid/` explaining the canonical raid state files.

## P0: Guild correctness

### P0.3 Fix invited-player membership state

**Finding:** `InviteToGuild()` writes a valid guild ID to the invitee, while `IsGuildMember()` treats any valid guild ID as membership. `JoinGuild()` rejects players that are already considered members, so an invited player can be blocked from accepting.

**Risk:** guild onboarding flow is functionally broken.

**Status:** Blocked

**Acceptance criteria:**
- Split invite state from accepted membership state.
- `IsGuildMember()` returns true only for accepted members.
- Add `HasGuildInvite()` or equivalent helper.
- Tests cover create, invite, accept, leave, promote, kick, and leader transfer.

**Suggested tests:**

```cpp
TEST(GuildTest, InvitedPlayerCanJoinGuild);
TEST(GuildTest, InviteDoesNotCountAsAcceptedMembership);
TEST(GuildTest, LeaderLeavingTransfersOwnershipToOfficer);
```

## P0: CI and release safety

### P0.4 Reduce GitHub Actions permissions

**Finding:** at least one PR-triggered workflow grants `contents: write` and exports `GITHUB_TOKEN` globally.

**Risk:** unnecessary write capability during build/test execution increases supply-chain blast radius.

**Status:** Blocked

**Acceptance criteria:**
- Default workflow permissions are `contents: read`.
- Write permissions are restricted to release/publish jobs only.
- `GITHUB_TOKEN` is scoped to the step that needs it, not global job env.
- Add `permissions:` to every workflow to avoid implicit defaults.

### P0.5 Add Linux test gate with `BUILD_TESTING=ON`

**Finding:** Linux packaging workflow disables tests even though the repo has a substantial test suite.

**Risk:** release packages can be produced while core tests fail.

**Status:** Planned

**Acceptance criteria:**
- Add a Linux test job separate from packaging.
- Test job runs on PRs and pushes to `master`.
- Test job builds with `BUILD_TESTING=ON` and runs `ctest --output-on-failure`.
- Packaging jobs require test job success.

## P1: Supply-chain hardening

### P1.1 Pin third-party GitHub Actions to commit SHAs

**Finding:** workflows use floating major tags and `latest` for actions.

**Risk:** workflow behavior can change without review.

**Status:** Planned

**Acceptance criteria:**
- Pin all third-party actions to full commit SHAs.
- Keep a comment beside each SHA with the human-readable version tag.
- Dependabot continues tracking GitHub Actions updates.

### P1.2 Expand dependency monitoring beyond GitHub Actions

**Finding:** Dependabot currently covers GitHub Actions only. The project also uses vcpkg and vendored dependencies under `3rdParty`.

**Risk:** C/C++ dependencies can age without automated visibility.

**Status:** Planned

**Acceptance criteria:**
- Add a documented monthly dependency bump process.
- Track vcpkg baseline updates with CI validation.
- Add dependency review or a generated dependency report artifact.
- Create a `docs/security/dependencies.md` ownership page.

### P1.3 Add CodeQL or equivalent static analysis

**Finding:** no repository CodeQL workflow was found during audit.

**Risk:** memory safety and unsafe API patterns rely only on review/manual testing.

**Status:** Planned

**Acceptance criteria:**
- Add CodeQL C/C++ analysis or equivalent static-analysis workflow.
- Run on PRs and scheduled weekly.
- Triage baseline findings and document accepted exceptions.

## P1: Netcode security and reliability

### P1.4 Replace fixed packet-encryption salt with per-session salt

**Finding:** packet encryption derives the key with Argon2id but uses a fixed salt string.

**Risk:** same password derives the same key across sessions, enabling more efficient offline guessing from captured traffic.

**Status:** Planned

**Acceptance criteria:**
- Generate a random salt for each secure session.
- Exchange salt during handshake before encrypted packets are required.
- Include protocol versioning/backward compatibility handling.
- Add tests for old/new handshake behavior and decryption failure paths.

### P1.5 Define network protocol compatibility gates

**Finding:** roadmap references protocol compatibility, packet loss recovery, host migration, and latency scenarios, but compatibility gates need to become release-blocking tests.

**Risk:** multiplayer changes can desync or break older clients without detection.

**Status:** In Progress

**Acceptance criteria:**
- Add wire-version documentation.
- Add deterministic state-sync tests at `150ms`, `250ms`, and `400ms` simulated latency.
- Add packet loss scenarios at `1%`, `5%`, and `10%`.
- Host migration smoke test reconnects within the documented target.

## P1: Lua mod sandbox and API maturity

### P1.6 Validate Lua package names before asset loading

**Finding:** Lua `require` builds asset paths from package names. The sandbox is intentionally narrow, but package-name validation should explicitly reject separators, traversal, empty segments, and unsupported characters.

**Risk:** malformed package names may escape intended module namespaces or create platform-specific path confusion.

**Status:** Planned

**Acceptance criteria:**
- Only allow package names matching `^[A-Za-z0-9_]+(\.[A-Za-z0-9_]+)*$` or an equivalent strict parser.
- Add tests for valid nested packages and invalid traversal-like names.
- Document allowed mod package naming rules.

### P1.7 Promote modding API from scaffold to operational

**Finding:** modding API docs mark versioned API enforcement, sandboxed hooks/perf budgets, canonical mod suite, and starter templates as scaffolded but not operational.

**Risk:** users may interpret roadmap scaffolding as a stable public API.

**Status:** Planned

**Acceptance criteria:**
- Publish an API surface map.
- Add CI check that fails undocumented public API changes.
- Add hook performance benchmark with target `< 1ms` average overhead per tick.
- Add at least 25 canonical mod fixtures.
- Add starter templates and packaging walkthroughs.

## P1: Documentation truth and roadmap hygiene

### P1.8 Add a feature maturity matrix

**Finding:** README roadmap presents large delivery areas, while several docs explicitly say capabilities are scaffolded but not operational.

**Risk:** contributors and users cannot tell what is implemented, tested, or release-ready.

**Status:** Planned

**Acceptance criteria:**
- Add a `docs/FEATURE_MATURITY.md` page.
- Track each roadmap capability as `scaffolded`, `compiled`, `tested`, `runtime-enabled`, or `release-ready`.
- Link it from the README roadmap section.
- Update status in the same PR as feature changes.

### P1.9 Fix broken roadmap links and missing docs

**Finding:** README links to roadmap documentation directories that may not contain the expected `README.md` or may be missing.

**Risk:** roadmap navigation breaks and contributors lose the trail.

**Status:** Planned

**Acceptance criteria:**
- Every README roadmap link resolves.
- Each roadmap area has an owner, status table, and acceptance evidence section.
- Add CI markdown link checking.

## P2: Raid content pipeline

### P2.1 Formalize raid state ownership and invariants

**Finding:** raid state, rules, routing, encounter schema, and tests exist, but canonical ownership should be explicit.

**Status:** In Progress

**Acceptance criteria:**
- Document canonical files for raid state/rules/routing/encounters.
- Add invariant tests for phase transitions, lockouts, member bitsets, checkpoint bits, and sequence monotonicity.
- Add replay tests to prove identical outcomes across consecutive runs.

### P2.2 Expand encounter compiler and schema validation

**Status:** Planned

**Acceptance criteria:**
- Define deterministic encounter script schema.
- Add at least 20 validation tests across boss phases, loot tables, and fail states.
- Add compiler/lint gate that blocks invalid bundles.
- Keep full bundle compile time under two minutes.

## P2: Guild economy and rewards

### P2.3 Implement treasury ledger invariants

**Finding:** guild progression exists, but treasury transaction types and zero-drift nightly audit are roadmap items, not operational guarantees.

**Status:** Planned

**Acceptance criteria:**
- Version transaction schemas.
- Add balance conservation checks.
- Add nightly seeded audit test with zero ledger drift.
- Persist audit output as CI artifact.

### P2.4 Implement weekly reward distribution branch coverage

**Status:** Planned

**Acceptance criteria:**
- Reward distribution covers join/leave mid-cycle, ties, inactivity, and duplicate claims.
- Add branch coverage gate for payout module.
- Add reconciliation report for admins.

## P2: Observability and operations

### P2.5 Add structured gameplay/network logs with trace IDs

**Status:** Planned

**Acceptance criteria:**
- Critical multiplayer paths emit trace IDs.
- Logs include session, peer, state sequence, and packet category where safe.
- No secrets, passwords, or raw authentication material are logged.

### P2.6 Add diagnostics bundles

**Status:** Planned

**Acceptance criteria:**
- Bundle captures logs, config, platform metadata, mod list, protocol version, and crash context.
- User-triggered export works on supported desktop platforms.
- Redaction rules are documented and tested.

## P3: Packaging and platform polish

### P3.1 Verify Hellfire MPQ packaging across platforms

**Finding:** recent changes package `mods/Hellfire.mpq` together with `devilutionx.mpq` for source distributions and some platform packages.

**Status:** In Progress

**Acceptance criteria:**
- Validate Windows, Linux, macOS, Vita, and PS4 packaging paths.
- Add packaging smoke tests that confirm expected assets exist.
- Document Hellfire MPQ layout expectations.

### P3.2 Standardize release artifact provenance

**Status:** Planned

**Acceptance criteria:**
- Add checksums for release artifacts.
- Document build inputs and dependency baselines.
- Attach provenance notes to release artifacts.

## Suggested milestone sequence

### Milestone 1: Make master trustworthy

- P0.1 Add missing mod API files to build.
- P0.2 Remove stale raid implementation.
- P0.3 Fix guild invite acceptance.
- P0.4 Reduce workflow permissions.
- P0.5 Add Linux test gate.

### Milestone 2: Harden the perimeter

- P1.1 Pin GitHub Actions.
- P1.2 Expand dependency monitoring.
- P1.3 Add static analysis.
- P1.6 Validate Lua package names.
- P1.8 Add feature maturity matrix.

### Milestone 3: Multiplayer/modding reliability

- P1.4 Per-session encryption salt.
- P1.5 Protocol compatibility gates.
- P1.7 Operational modding API gates.
- P2.1 Raid invariant expansion.

### Milestone 4: Economy, ops, and release readiness

- P2.3 Treasury ledger invariants.
- P2.4 Weekly reward coverage.
- P2.5 Structured trace logs.
- P2.6 Diagnostics bundles.
- P3.1 Packaging smoke tests.
- P3.2 Artifact provenance.

## Definition of done for roadmap items

A roadmap item is done only when all of the following are true:

- Implementation is merged.
- Tests or validation gates are merged.
- Documentation reflects runtime behavior.
- CI passes on affected platforms.
- Any security-sensitive behavior has a failure-mode test.
- Feature maturity status is updated.

## Review cadence

- Review this roadmap at the start of every release cycle.
- Update statuses in the same PR that changes implementation state.
- Close or split items that remain too broad after one milestone.
