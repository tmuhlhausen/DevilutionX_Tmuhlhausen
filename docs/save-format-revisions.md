# Save format revisions

## Player block revision (`Source/loadsave.cpp`)

`PlayerSaveFormatRevision` tracks extensions written into the "available bytes" tail of the player save block.

### Revision 0
- Legacy format.
- Nephilim progression fields are absent.
- Loader migration behavior:
  - `pNephilimLevel = 0`
  - `pNephilimExperience = 0`

### Revision 1
- Adds Nephilim progression fields to the player save tail:
  - `pNephilimLevel` (`uint8_t`)
  - `pNephilimExperience` (`uint32_t`)
- Layout in tail section:
  1. `revision` (`uint8_t`)
  2. `pNephilimLevel` (`uint8_t`)
  3. alignment (`2 bytes`)
  4. `pNephilimExperience` (`uint32_t`)
  5. remaining reserved padding (`12 bytes`)

## Packed player revision (`Source/pack.cpp`)

`PackedPlayerSaveRevision` tracks the same Nephilim extension in `PlayerPack` and allows `UnPackPlayer` to default old packed characters to Nephilim level/XP = 0.
