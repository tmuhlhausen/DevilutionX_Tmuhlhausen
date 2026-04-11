#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "guild/guild.hpp"
#include "guild/guild_progression.hpp"
#include "raid/raid.hpp"

namespace devilution {

constexpr size_t RaidDifficultyProgressionCount = 3;
constexpr size_t MaxRaidPlayerRewardClaims = 256;
constexpr size_t MaxRaidGuildRewardClaims = 256;
constexpr uint8_t NoRaidCheckpointBoss = 0xFF;

struct RaidDifficultyProgressState {
	uint32_t lockoutWeek = 0;
	uint16_t attemptsThisWeek = 0;
	uint16_t bestClearDurationSeconds = 0;
	uint8_t bestBossesDefeated = 0;
	uint8_t checkpointBossIndex = NoRaidCheckpointBoss;
	uint64_t checkpointObjectiveBits = 0;
	std::array<uint32_t, MaxRaidTimers> checkpointTimersMs {};
	uint32_t bestClearRaidId = 0;
};

struct RaidRewardClaim {
	uint64_t token = 0;
	uint32_t guildId = 0;
	uint32_t raidId = 0;
	uint8_t playerId = 0;
	GuildActivityType activity = GuildActivityType::DungeonClear;
	uint16_t milestone = 0;
	uint16_t quantity = 0;
};

struct RaidProgressionPersistedState {
	std::array<RaidDifficultyProgressState, RaidDifficultyProgressionCount> difficulties {};
	std::array<RaidRewardClaim, MaxRaidPlayerRewardClaims> playerClaims {};
	std::array<RaidRewardClaim, MaxRaidGuildRewardClaims> guildClaims {};
	uint16_t usedPlayerClaims = 0;
	uint16_t usedGuildClaims = 0;
};

void ResetRaidProgression();
RaidProgressionPersistedState GetRaidProgressionPersistedState();
void ApplyRaidProgressionPersistedState(const RaidProgressionPersistedState &state);

bool BeginRaidAttempt(RaidDifficulty difficulty, uint32_t currentWeek);
void SetRaidLockoutForWeek(RaidDifficulty difficulty, uint32_t currentWeek);
bool IsRaidDifficultyLocked(RaidDifficulty difficulty, uint32_t currentWeek);

bool SaveRaidCheckpoint(RaidDifficulty difficulty, uint8_t bossIndex, uint64_t objectiveBits, const std::array<uint32_t, MaxRaidTimers> &timersMs);
void ClearRaidCheckpoint(RaidDifficulty difficulty);

void RecordRaidDungeonClear(uint32_t raidId, RaidDifficulty difficulty, GuildId guildId, uint8_t guildLevel, uint8_t bossesDefeated, uint16_t clearDurationSeconds, const std::vector<uint8_t> &participantIds, uint32_t currentWeek);
const std::vector<RaidRewardClaim> &GetRaidRewardClaims();

} // namespace devilution
