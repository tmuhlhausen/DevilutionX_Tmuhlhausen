#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace devilution {

constexpr size_t MaxRaidBosses = 8;
constexpr size_t MaxRaidTimers = 4;

struct RaidId {
	uint32_t value = 0;

	[[nodiscard]] bool IsValid() const
	{
		return value != 0;
	}

	[[nodiscard]] bool operator==(const RaidId &other) const = default;
};

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

enum class RaidResult : uint8_t {
	None,
	Success,
	Failure,
};

enum class RaidLockout : uint8_t {
	None,
	Active,
	Expired,
};

using RaidLockoutState = RaidLockout;

enum class RaidEncounterState : uint8_t {
	NotStarted,
	Active,
	Defeated,
	Failed,
};

enum RaidRoleFlags : uint32_t {
	RaidRoleNone = 0,
	RaidRoleTank = 1 << 0,
	RaidRoleHealer = 1 << 1,
	RaidRoleDamage = 1 << 2,
	RaidRoleSupport = 1 << 3,
};

struct RaidMemberSnapshot {
	uint8_t playerId = 0;
	bool isAlive = false;
	uint32_t contribution = 0;
	uint32_t roleFlags = RaidRoleNone;
};

struct RaidEncounterEvent {
	uint8_t bossIndex = 0;
	RaidEncounterState state = RaidEncounterState::NotStarted;
	uint64_t objectiveBitsToSet = 0;
	uint32_t checkpointBitsToSet = 0;
	std::array<uint32_t, MaxRaidTimers> timersMs {};
	bool updateTimers = false;
};

} // namespace devilution
