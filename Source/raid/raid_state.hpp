#pragma once

#include <array>
#include <cstdint>

#include "raid/raid_types.hpp"

namespace devilution {

struct RaidInstanceState {
	RaidId raidId {};
	RaidDifficulty difficulty = RaidDifficulty::None;
	RaidPhase phase = RaidPhase::Inactive;
	RaidResult result = RaidResult::None;
	RaidLockout lockout = RaidLockout::None;
	uint32_t instanceSeed = 0;
	std::array<RaidEncounterState, MaxRaidBosses> bossStates {};
	uint64_t objectiveBits = 0;
	uint32_t checkpointBits = 0;
	uint32_t joinedMemberBits = 0;
	uint32_t readyMemberBits = 0;
	std::array<uint32_t, MaxRaidTimers> timersMs {};
	uint32_t lockoutExpirationTick = 0;
	uint32_t snapshotRevision = 0;
	uint32_t sequence = 0;
};

void InitializeRaidSubsystem();
void ResetRaidSubsystem();

void ResetRaid(RaidInstanceState &state, uint32_t newInstanceSeed);
[[nodiscard]] bool AdvanceRaidStateSequence(RaidInstanceState &state);
[[nodiscard]] uint8_t GetRaidMemberCount(const RaidInstanceState &state);
[[nodiscard]] uint8_t GetRaidReadyCount(const RaidInstanceState &state);
[[nodiscard]] bool IsRaidMemberJoined(const RaidInstanceState &state, uint8_t playerId);
[[nodiscard]] bool SetRaidMemberJoined(RaidInstanceState &state, uint8_t playerId, bool joined);
[[nodiscard]] bool SetRaidMemberReady(RaidInstanceState &state, uint8_t playerId, bool ready);
[[nodiscard]] bool ToggleRaidMemberReady(RaidInstanceState &state, uint8_t playerId);

void ResetActiveRaidState();
[[nodiscard]] RaidInstanceState GetActiveRaidState();
[[nodiscard]] bool ApplyActiveRaidStateSnapshot(const RaidInstanceState &state);

[[nodiscard]] bool ApplyEncounterEvent(RaidInstanceState &state, const RaidEncounterEvent &event);
[[nodiscard]] bool CompleteRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick);
[[nodiscard]] bool FailRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick);

} // namespace devilution
