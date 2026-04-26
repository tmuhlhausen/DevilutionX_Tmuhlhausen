# Feature Maturity Matrix

This matrix tracks fork-specific roadmap capabilities by runtime maturity. It is a release-readiness map, not a wish list.

## Status definitions

| Status | Meaning |
|---|---|
| `Scaffolded` | Design docs, types, or placeholders exist, but runtime behavior is not complete. |
| `Compiled` | Code is included in the build and links successfully. |
| `Tested` | Automated tests cover expected behavior and important failure modes. |
| `Runtime-enabled` | Feature is wired into gameplay, UI, networking, or packaging paths. |
| `Release-ready` | Feature is documented, tested, observable, and guarded by CI/release gates. |

## Matrix

| Area | Capability | Current status | Release gate |
|---|---|---:|---|
| Build integrity | Guild/raid mod API sources are included in the main build | `Compiled` | Linux + Windows CI pass with `BUILD_TESTING=ON` where applicable. |
| CI | Linux test job runs before packaging | `Runtime-enabled` | Package build depends on test success. |
| CI | Workflow token permissions are read-only by default | `Runtime-enabled` | Write permissions are limited to release-only jobs. |
| Guilds | Create/invite/join/leave/promote/kick flows | `Tested` | Regression tests cover pending invites and ownership transfer. |
| Guild economy | Treasury ledger and weekly rewards | `Scaffolded` | Zero-drift ledger audit and payout branch coverage. |
| Raid pipeline | Canonical raid state/rules/routing implementation | `Tested` | Invariant and replay tests cover phase, lockout, member, and checkpoint behavior. |
| Raid content | Encounter schema and weekly modifiers | `Tested` | Schema lint blocks invalid bundles. |
| Netcode | Runtime hook helpers and safety checks | `Compiled` | Protocol compatibility, latency, packet loss, and host migration tests. |
| Lua modding | Sandboxed `require` package name validation | `Tested` | Reject traversal, separators, empty segments, and unsupported characters. |
| Lua modding | Versioned public modding API | `Scaffolded` | CI blocks undocumented public API changes. |
| Observability | Structured gameplay/network logs with trace IDs | `Scaffolded` | Critical multiplayer paths emit trace IDs without leaking secrets. |
| Packaging | Hellfire MPQ package coverage | `In Progress` | Platform package smoke tests confirm expected assets. |
| Release provenance | Checksums and build input notes | `Scaffolded` | Release artifacts include checksums and provenance notes. |

## Maintenance rules

- Update this file in the same PR that changes a feature's maturity.
- Do not mark a feature `Release-ready` without automated validation and user-facing docs.
- Prefer downgrading a status when evidence disappears over keeping optimistic labels.
- Link feature-specific proof in the relevant docs directory when available.

## Related roadmap

See [`docs/MASTER_ROADMAP.md`](MASTER_ROADMAP.md) for priority order, acceptance criteria, and milestone sequencing.
