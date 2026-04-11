#include <gtest/gtest.h>

#include "raid/raid.hpp"

namespace devilution {
namespace {

RaidInstanceState MakeFormingRaid()
{
	RaidInstanceState state {};
	state.raidId.value = 42;
	state.difficulty = RaidDifficulty::Hell;
	state.phase = RaidPhase::Forming;
	return state;
}

TEST(RaidStateTest, JoinGuardRejectsDeadOrRolelessMembers)
{
	const RaidInstanceState state = MakeFormingRaid();
	const RaidMemberSnapshot deadMember { .playerId = 1, .isAlive = false, .contribution = 0, .roleFlags = RaidRoleDamage };
	const RaidMemberSnapshot rolelessMember { .playerId = 1, .isAlive = true, .contribution = 0, .roleFlags = RaidRoleNone };

	EXPECT_FALSE(CanJoinRaid(state, deadMember, 0, 8));
	EXPECT_FALSE(CanJoinRaid(state, rolelessMember, 0, 8));
}

TEST(RaidStateTest, JoinGuardRejectsFullLockedOrInProgressRaid)
{
	RaidInstanceState state = MakeFormingRaid();
	const RaidMemberSnapshot member { .playerId = 1, .isAlive = true, .contribution = 0, .roleFlags = RaidRoleSupport };

	EXPECT_FALSE(CanJoinRaid(state, member, 8, 8));

	state.lockoutState = RaidLockoutState::Active;
	EXPECT_FALSE(CanJoinRaid(state, member, 0, 8));

	state.lockoutState = RaidLockoutState::None;
	state.phase = RaidPhase::InProgress;
	EXPECT_FALSE(CanJoinRaid(state, member, 0, 8));
}

TEST(RaidStateTest, StartGuardRequiresValidIdentityAndReadiness)
{
	RaidInstanceState state = MakeFormingRaid();

	EXPECT_FALSE(CanStartRaid(state, 1, 2));
	EXPECT_TRUE(CanStartRaid(state, 2, 2));

	state.raidId.value = 0;
	EXPECT_FALSE(CanStartRaid(state, 2, 2));

	state.raidId.value = 42;
	state.difficulty = RaidDifficulty::None;
	EXPECT_FALSE(CanStartRaid(state, 2, 2));
}

TEST(RaidStateTest, EncounterEventAppliesOnlyDuringInProgress)
{
	RaidInstanceState state = MakeFormingRaid();
	RaidEncounterEvent event {};
	event.bossIndex = 0;
	event.state = RaidEncounterState::Active;
	event.objectiveBitsToSet = 0b101;
	event.updateTimers = true;
	event.timersMs = { 1000, 2000, 0, 0 };

	EXPECT_FALSE(ApplyEncounterEvent(state, event));

	state.phase = RaidPhase::InProgress;
	ASSERT_TRUE(ApplyEncounterEvent(state, event));
	EXPECT_EQ(state.bossStates[0], RaidEncounterState::Active);
	EXPECT_EQ(state.objectiveBits & 0b101, 0b101);
	EXPECT_EQ(state.timersMs[0], 1000);
	EXPECT_GT(state.snapshotRevision, 0u);
}

} // namespace
} // namespace devilution
