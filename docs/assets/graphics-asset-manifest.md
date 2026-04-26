# Graphics Asset Manifest

## 1) Runtime MPQ asset matrix (required / optional / by-platform)

| Asset | Purpose | Required (Base Diablo) | Required (Hellfire Mode) | Optional | Platform notes |
|---|---|---:|---:|---:|---|
| `diabdat.mpq` (or `DIABDAT.MPQ`) | Core Diablo game data | ✅ | ✅ (for Diablo mode) | ❌ | Loaded first when available; fallback can be `spawn.mpq`. |
| `spawn.mpq` | Spawn/demo data fallback | ✅* | ✅* | ✅ | Used only when `diabdat.mpq` is not found. |
| `devilutionx.mpq` | DevilutionX UI/fonts/assets pack | ✅ | ✅ | ❌ | Generally required; startup checks fail if out-of-date/missing UI resources. On **3DS**, do **not** copy this to SD (platform package provides assets path). |
| `fonts.mpq` | Extra glyphs / locale coverage | ⚠️ locale-dependent | ⚠️ locale-dependent | ✅ | Required for locales that need tall/supplemental fonts (e.g., CJK); English can run without it. |
| `hellfire.mpq` | Hellfire expansion core data | ❌ | ✅ | ✅ | Required when Hellfire is forced/enabled. |
| `hfmonk.mpq` | Hellfire monk class assets | ❌ | ✅ | ❌ | Fatal error in Hellfire mode if missing. |
| `hfmusic.mpq` | Hellfire music assets | ❌ | ✅ | ❌ | Fatal error in Hellfire mode if missing. |
| `hfvoice.mpq` | Hellfire voice assets | ❌ | ✅ | ❌ | Fatal error in Hellfire mode if missing. |
| `<lang>.mpq` (e.g., `pl.mpq`, `ru.mpq`) | Translation/voice packs | ❌ | ❌ | ✅ | Loaded for non-`en` language code if present. |

### Platform-specific load behavior (graphics-relevant)

- `LoadCoreArchives()` skips eager `devilutionx.mpq` pre-load on `__ANDROID__`, `__APPLE__`, `__3DS__`, and `__SWITCH__`; other platforms load it early for font/UI error-message readiness.
- `fonts.mpq` is always attempted as an extra-font archive, then locale rules decide if it is mandatory for the active language.
- 3DS installation docs explicitly note that `devilutionx.mpq` should not be copied to SD for runtime.

---

## 2) Test fixture PNG inventory and source paths

### Golden PNG fixtures

Directory: `test/fixtures/text_render_integration_test/`

- `basic.png`
- `basic-colors.png`
- `horizontal_overflow.png`
- `horizontal_overflow-colors.png`
- `kerning_fit_spacing.png`
- `kerning_fit_spacing-colors.png`
- `kerning_fit_spacing__align_center.png`
- `kerning_fit_spacing__align_center-colors.png`
- `kerning_fit_spacing__align_center__newlines.png`
- `kerning_fit_spacing__align_center__newlines_in_fmt-colors.png`
- `kerning_fit_spacing__align_center__newlines_in_value-colors.png`
- `kerning_fit_spacing__align_right.png`
- `kerning_fit_spacing__align_right-colors.png`
- `vertical_overflow.png`
- `vertical_overflow-colors.png`
- `cursor-start.png`
- `cursor-middle.png`
- `cursor-end.png`
- `multiline_cursor-end_first_line.png`
- `multiline_cursor-start_second_line.png`
- `multiline_cursor-middle_second_line.png`
- `multiline_cursor-end_second_line.png`
- `highlight-partial.png`
- `highlight-full.png`
- `multiline_highlight.png`

### Source paths and generation mapping

- Test definition/source: `test/text_render_integration_test.cpp`
- Golden file root constant: `FixturesPath = "test/fixtures/text_render_integration_test/"`
- Generated actual output pattern during test run: `<fixture-name>-Actual.png`
- Expected comparison target pattern: `<fixture-name>.png`

---

## 3) Font/palette dependencies for render snapshot tests

`test/text_render_integration_test.cpp` depends on these graphics assets:

1. **Palette dependency**
   - Loads `ui_art\\diablo.pal` via `LoadFileInMem`.
   - Converts palette bytes to `SDL_Palette` and assigns it to output surfaces before draw/compare.

2. **Font/text rendering dependency**
   - Exercises `DrawString` / `DrawStringWithColors` in `engine/render/text_render.hpp`.
   - Uses UI flags and color indices (`UiFlags::*`, `PAL8_*`) that rely on valid UI font and palette resources.

3. **PNG decode/encode dependency**
   - Writes actual frames with `WriteSurfaceToFilePng`.
   - Re-loads both actual and expected images with `LoadPNG` and compares indexed pixels.

4. **Locale/font archive behavior (runtime relevance to snapshots)**
   - Non-English locales may require `fonts.mpq`; if missing, locale is forced back to English with an explicit UI warning.
   - Startup also validates out-of-date archive states for `devilutionx.mpq` and `fonts.mpq`.

---

## 4) Verification command snippets for CI artifact checksums

Use these snippets to generate and verify checksum manifests for graphics-related artifacts (PNGs, packaged MPQs, release bundles).

### Linux (sha256sum)

```bash
# Generate checksums for fixture PNGs (sorted for deterministic manifest output)
find test/fixtures/text_render_integration_test -maxdepth 1 -name '*.png' -print0 \
  | sort -z \
  | xargs -0 sha256sum > graphics-fixtures.sha256

# Verify checksums
sha256sum --check graphics-fixtures.sha256
```

### macOS (shasum)

```bash
find test/fixtures/text_render_integration_test -maxdepth 1 -name '*.png' -print0 \
  | sort -z \
  | xargs -0 shasum -a 256 > graphics-fixtures.sha256

shasum -a 256 --check graphics-fixtures.sha256
```

### Windows PowerShell (Get-FileHash)

```powershell
# Generate manifest
Get-ChildItem test/fixtures/text_render_integration_test/*.png |
  Sort-Object Name |
  Get-FileHash -Algorithm SHA256 |
  ForEach-Object { "$($_.Hash.ToLower()) *$($_.Path.Replace('\\','/'))" } |
  Set-Content graphics-fixtures.sha256

# Verify manifest
Get-Content graphics-fixtures.sha256 | ForEach-Object {
  $parts = $_ -split ' \*', 2
  $expected = $parts[0]
  $path = $parts[1]
  $actual = (Get-FileHash -Algorithm SHA256 $path).Hash.ToLower()
  if ($actual -ne $expected) { throw "Checksum mismatch: $path" }
}
```

### GitHub Actions example step

```yaml
- name: Verify graphics fixture checksums
  run: |
    sha256sum --check graphics-fixtures.sha256
```
