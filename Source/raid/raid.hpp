#pragma once

#include "raid/raid_rules.hpp"
#include "raid/raid_state.hpp"
#include "raid/raid_types.hpp"

namespace devilution {

constexpr size_t MaxRaidBosses = 8;
constexpr size_t MaxRaidTimers = 4;

enum class RaidDifficulty : uint8_t {
	None,
	Normal,
	Nightmare,
	Hell,
};

enum class RaidPhase : uint8_t {
	Inactive,
	Forming,
	InProgress,
	Completed,
	Failed,
	LockedOut,
};

enum class RaidEncounterState : uint8_t {
	NotStarted,
	Active,
	Defeated,
	Failed,
};

enum class RaidLockoutState : uint8_t {
	None,
	Active,
	Expired,
};

enum RaidRoleFlags : uint32_t {
	RaidRoleNone = 0,
	RaidRoleTank = 1 << 0,
	RaidRoleHealer = 1 << 1,
	RaidRoleDamage = 1 << 2,
	RaidRoleSupport = 1 << 3,
};

struct RaidId {
	uint32_t value = 0;

	[[nodiscard]] bool IsValid() const
	{
		return value != 0;
	}

	[[nodiscard]] bool operator==(const RaidId &other) const = default;
};

struct RaidMemberSnapshot {
	uint8_t playerId = 0;
	bool isAlive = false;
	uint32_t contribution = 0;
	uint32_t roleFlags = RaidRoleNone;
};

struct RaidInstanceState {
	RaidId raidId {};
	RaidDifficulty difficulty = RaidDifficulty::None;
	RaidPhase phase = RaidPhase::Inactive;
	RaidLockoutState lockoutState = RaidLockoutState::None;
	uint32_t instanceSeed = 0;
	std::array<RaidEncounterState, MaxRaidBosses> bossStates {};
	uint64_t objectiveBits = 0;
	std::array<uint32_t, MaxRaidTimers> timersMs {};
	uint32_t lockoutExpirationTick = 0;
	uint32_t snapshotRevision = 0;
	uint32_t reservedNetSync = 0;
};

struct RaidEncounterEvent {
	uint8_t bossIndex = 0;
	RaidEncounterState state = RaidEncounterState::NotStarted;
	uint64_t objectiveBitsToSet = 0;
	std::array<uint32_t, MaxRaidTimers> timersMs {};
	bool updateTimers = false;
};

struct RaidLobbyUiState {
	uint32_t joinedMask = 0;
	uint32_t readyMask = 0;
	std::array<uint8_t, 4> roleSlots {};
	uint8_t attemptsLeft = 0;
};

[[nodiscard]] bool CanJoinRaid(const RaidInstanceState &state, const RaidMemberSnapshot &member, uint8_t activeMemberCount, uint8_t maxMembers);
[[nodiscard]] bool CanStartRaid(const RaidInstanceState &state, uint8_t readyMemberCount, uint8_t minimumMemberCount);
[[nodiscard]] bool ApplyEncounterEvent(RaidInstanceState &state, const RaidEncounterEvent &event);
[[nodiscard]] bool CompleteRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick);
[[nodiscard]] bool FailRaid(RaidInstanceState &state, uint32_t lockoutExpirationTick);
void ResetRaid(RaidInstanceState &state, uint32_t newInstanceSeed);

void ResetActiveRaidState();
RaidInstanceState GetActiveRaidState();
void ApplyActiveRaidStateSnapshot(const RaidInstanceState &state);
RaidLobbyUiState GetActiveRaidLobbyUiState();
void ApplyActiveRaidLobbyUiState(const RaidLobbyUiState &state);

} // namespace devilution
