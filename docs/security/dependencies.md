# Dependency Stewardship

This page defines how dependency updates are owned, reviewed, and validated for this fork.

## Scope

Dependency stewardship covers:

- GitHub Actions used by CI and release workflows.
- vcpkg manifest dependencies in `vcpkg.json`.
- Vendored third-party code under `3rdParty/`.
- Platform package scripts and bootstrap scripts under `Packaging/`.
- Generated dependency artifacts from CI.

## Ownership

| Area | Primary owner | Review expectation |
|---|---|---|
| GitHub Actions | Build/Release maintainers | Review monthly or when Dependabot opens an update. |
| vcpkg baseline and manifest | Build/Release maintainers | Review monthly with full CI. |
| Networking and crypto libraries | Netcode maintainers | Review before release and after security advisories. |
| Lua/modding dependencies | Modding maintainers | Review before modding API milestones. |
| Vendored libraries | Area maintainers | Review when upstream releases or CVEs affect the component. |

## Monthly dependency bump ritual

1. Check Dependabot PRs for GitHub Actions.
2. Review vcpkg baseline drift against the current manifest.
3. Build and test with `BUILD_TESTING=ON` on Linux and Windows.
4. Review security advisories for `fmt`, `bzip2`, `lua`, `magic-enum`, `libsodium`, `gtest`, `benchmark`, SDL, SDL_image, and vendored `3rdParty` libraries.
5. Record notable updates in the PR body.
6. Mark skipped updates with a reason and revisit date.

## Validation gate

A dependency update is mergeable only when:

- A clean configure/build/test run passes on affected platforms.
- Release/package workflows still produce expected artifacts.
- Security-sensitive dependencies have explicit review notes.
- Any breaking upgrade includes a migration note.

## vcpkg baseline process

When changing the `builtin-baseline` in `vcpkg.json`:

1. Explain why the baseline is moving.
2. List dependency versions that changed materially.
3. Run CI with `BUILD_TESTING=ON`.
4. Confirm packet encryption builds with the `encryption` feature enabled.
5. Confirm tests build with the `tests` feature enabled.

## Vendored dependency process

For `3rdParty/` updates:

1. Link the upstream release or commit.
2. Confirm license compatibility with the Sustainable Use License constraints.
3. Add any local patch notes.
4. Run affected tests and platform smoke checks.
5. Update attribution or notices when needed.

## Security response

For a dependency vulnerability:

1. Determine whether the vulnerable code is compiled into default builds.
2. Determine whether the vulnerable path is reachable from untrusted input.
3. Patch, disable, or mitigate the affected dependency.
4. Add a regression test or release note when applicable.
5. Record the outcome in the issue or PR.

## CI dependency inventory artifact

The `Dependency Inventory` workflow runs `tools/dependency_inventory.py` on pushes, pull requests, a weekly schedule, and manual dispatch.

It uploads a `dependency-inventory` artifact containing:

- `dependency-inventory.json`
- `dependency-inventory.md`

The report uses repository-local metadata only:

- `vcpkg.json`
- GitHub Actions `uses:` references from `.github/workflows/`
- Vendored dependency directories under `3rdParty/`

Review this artifact during dependency bumps, release preparation, and security triage.

## CI artifacts to add next

Future improvements should generate and upload:

- A release artifact dependency summary.
- A security-review checklist for crypto/networking changes.
- Optional CVE/advisory annotations for the generated inventory.
