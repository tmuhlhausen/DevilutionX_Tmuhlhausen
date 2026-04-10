#include "raid/raid_routing.hpp"

#include <algorithm>
#include <limits>

namespace devilution {

namespace {

uint32_t MixRaidId(uint32_t raidId)
{
	raidId ^= raidId >> 16;
	raidId *= 0x7feb352dU;
	raidId ^= raidId >> 15;
	raidId *= 0x846ca68bU;
	raidId ^= raidId >> 16;
	return raidId;
}

} // namespace

uint8_t ComputeRaidMultiplayerLevel(uint32_t raidId, uint8_t encounterIndex, uint8_t raidMultiplayerBase)
{
	const uint8_t normalizedEncounter = encounterIndex % RaidEncounterBuckets;
	const uint8_t raidBucket = MixRaidId(raidId) % MaxActiveRaidInstances;
	const uint16_t offset = raidBucket * RaidEncounterBuckets + normalizedEncounter;
	const uint16_t level = raidMultiplayerBase + offset;

	if (level > std::numeric_limits<uint8_t>::max())
		return std::numeric_limits<uint8_t>::max();

	return static_cast<uint8_t>(level);
}

} // namespace devilution
