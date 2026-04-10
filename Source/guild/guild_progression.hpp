#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "guild/guild.hpp"
#include "tables/itemdat.h"

namespace devilution {

constexpr size_t GuildActivityTypeCount = 5;
constexpr size_t MaxGuildProgressionMilestones = 64;
constexpr size_t MaxGuildRewardDedupKeys = 128;

struct GrantedGuildReward {
	uint64_t dedupKey = 0;
	GuildActivityType activity = GuildActivityType::DungeonClear;
	uint16_t milestone = 0;
	uint16_t quantity = 0;
};

struct GuildProgressionPersistedState {
	GuildId guildId {};
	std::array<uint32_t, GuildActivityTypeCount> counters {};
	std::array<uint64_t, GuildActivityTypeCount> completedMilestones {};
	std::array<uint64_t, MaxGuildRewardDedupKeys> dedupKeys {};
	uint16_t usedDedupKeys = 0;
};

void ResetGuildProgression();
GuildProgressionPersistedState GetGuildProgressionPersistedState();
void ApplyGuildProgressionPersistedState(const GuildProgressionPersistedState &state);

void EmitGuildActivityEvent(GuildId guildId, GuildActivityType activity, uint16_t amount, uint8_t guildLevel, uint32_t raidId);
void HandleRaidCompletionForGuild(uint32_t raidId, GuildId guildId, uint8_t guildLevel, uint8_t bossKillCount);

const std::vector<GrantedGuildReward> &GetGrantedGuildRewards();

} // namespace devilution
