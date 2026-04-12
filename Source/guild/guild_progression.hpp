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
constexpr size_t MaxGuildSeasonSnapshots = 16;
constexpr size_t MaxGuildSeasonClaimDedupKeys = 256;

enum class GuildRewardKind : uint8_t {
	LegendaryTemplate = 0,
	TokenCurrency = 1,
	TierStoreUnlock = 2,
};

struct GuildRankingSnapshot {
	uint32_t seasonId = 0;
	uint32_t snapshotId = 0;
	uint32_t accumulatedActivity = 0;
	uint16_t tier = 0;
	uint32_t prestigePoints = 0;
};

struct GrantedGuildReward {
	uint64_t dedupKey = 0;
	uint64_t claimDedupKey = 0;
	GuildActivityType activity = GuildActivityType::DungeonClear;
	uint16_t milestone = 0;
	uint16_t quantity = 0;
	GuildRewardKind rewardKind = GuildRewardKind::LegendaryTemplate;
	uint16_t tokenCurrency = 0;
	uint16_t tierStoreTier = 0;
	uint32_t seasonId = 0;
	bool seasonLimited = true;
};

struct GuildProgressionPersistedState {
	GuildId guildId {};
	std::array<uint32_t, GuildActivityTypeCount> counters {};
	std::array<uint32_t, GuildActivityTypeCount> seasonCounterBaseline {};
	std::array<uint64_t, GuildActivityTypeCount> completedMilestones {};
	std::array<uint64_t, MaxGuildRewardDedupKeys> dedupKeys {};
	std::array<uint64_t, MaxGuildSeasonClaimDedupKeys> claimDedupKeys {};
	std::array<GuildRankingSnapshot, MaxGuildSeasonSnapshots> rankingSnapshots {};
	uint32_t seasonId = 1;
	uint32_t seasonResetIntervalWeeks = 4;
	uint32_t nextSeasonResetWeek = 4;
	uint32_t seasonTransitions = 0;
	uint32_t prestigePoints = 0;
	uint16_t prestigeLevel = 0;
	uint16_t usedDedupKeys = 0;
	uint16_t usedClaimDedupKeys = 0;
	uint8_t usedRankingSnapshots = 0;
};

void ResetGuildProgression();
GuildProgressionPersistedState GetGuildProgressionPersistedState();
void ApplyGuildProgressionPersistedState(const GuildProgressionPersistedState &state);

void EmitGuildActivityEvent(GuildId guildId, GuildActivityType activity, uint16_t amount, uint8_t guildLevel, uint32_t raidId);
void HandleRaidCompletionForGuild(uint32_t raidId, GuildId guildId, uint8_t guildLevel, uint8_t bossKillCount);
void AdvanceGuildSeasonIfNeeded(uint32_t currentWeek);
uint16_t CalculateGuildSeasonTier();
void CaptureGuildRankingSnapshot(uint32_t snapshotId);

const std::vector<GrantedGuildReward> &GetGrantedGuildRewards();

} // namespace devilution
