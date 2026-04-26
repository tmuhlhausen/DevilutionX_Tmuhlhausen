# Modding API

## Scope and goals

This document defines the baseline for a stable, versioned modding interface with sandboxed hooks, compatibility guarantees, and rapid onboarding for mod authors.

Goals:
- Formalize and version public API boundaries.
- Enforce compatibility policy with CI safeguards.
- Provide safe, performant script hooks for core gameplay events.
- Minimize setup friction for new mod developers.

## Lua package naming

Lua packages loaded through `require()` are resolved into asset paths. To keep module loading deterministic and confined to the intended module namespace, package names must follow this grammar:

```text
[A-Za-z0-9_]+(\.[A-Za-z0-9_]+)*
```

Allowed examples:

- `inspect`
- `devilutionx.events`
- `mods.My_Mod_01.init`

Blocked categories:

- Empty names or empty package segments.
- Filesystem separators.
- Parent-directory traversal markers.
- Whitespace and punctuation other than `_` and `.`.
- Hyphens in package/module identifiers.

Use underscores instead of hyphens for Lua package/module identifiers.

## Acceptance criteria mapped from README.md roadmap bullets

| Roadmap bullet (README.md) | Acceptance criteria | Evidence |
|---|---|---|
| Publish a versioned API surface map and semantic compatibility policy, with CI checks blocking undocumented public API changes. | API inventory is versioned; compatibility policy is published; CI fails when public symbols change without documentation updates. | API map docs + CI change-detection report. |
| Deliver sandboxed script hooks for combat, loot, and events, including performance budgets (`< 1ms` average hook overhead per tick). | Hook points are implemented for combat/loot/events; sandbox policy validates unsafe operations; benchmark suite enforces <1ms average overhead. | Hook integration tests + performance benchmark artifact. |
| Add an integration test suite of `25+` canonical mods to validate API behavior across patch upgrades. | Canonical mod suite contains at least 25 fixtures; patch-upgrade CI job runs compatibility matrix and blocks regressions. | `test/modding_api_integration_*` outputs + matrix results. |
| Provide starter mod templates and packaging docs that reduce new-mod setup time to under `15 minutes` for first-time contributors. | Starter templates cover minimal, content, and scripted mods; setup walkthrough validated by first-time contributor dry run in <15 minutes. | Template repo/docs + onboarding time trial report. |

## Required tests and file ownership

Required tests:
- API surface diff/compatibility checks.
- Hook sandbox policy tests.
- Hook performance budget benchmarks.
- Canonical mod integration compatibility suite.
- Starter template smoke tests.
- Lua package-name validation tests.

Suggested ownership:
- Primary: Modding Platform maintainers.
- Secondary: Gameplay systems for hook correctness.
- Docs owner: `docs/modding-api/` maintainers for compatibility policy and templates.

## Status

| Capability | Scaffold Completed | Feature Operational | Notes |
|---|---|---|---|
| Versioned API map & compatibility policy | ✅ | ⛔ | Documentation scaffolded; policy enforcement wiring pending. |
| Sandboxed hooks with perf budgets | ✅ | ⛔ | Requirements documented; runtime hooks/benchmarks pending. |
| Lua package-name validation | ✅ | ✅ | Runtime guard and regression tests added. |
| 25+ canonical mod integration suite | ✅ | ⛔ | Test targets defined; fixture implementation pending. |
| Starter templates & packaging docs | ✅ | ⛔ | Onboarding outcomes documented; templates pending. |
