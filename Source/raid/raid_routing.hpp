#pragma once

#include <cstddef>
#include <cstdint>

namespace devilution {

constexpr uint8_t MaxActiveRaidInstances = 32;
constexpr uint8_t RaidEncounterBuckets = 16;
constexpr size_t RaidMultiplayerLevelSpan = MaxActiveRaidInstances * RaidEncounterBuckets;

[[nodiscard]] uint8_t ComputeRaidMultiplayerLevel(uint32_t raidId, uint8_t encounterIndex, uint8_t raidMultiplayerBase);

} // namespace devilution
