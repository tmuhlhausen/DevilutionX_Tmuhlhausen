#include <gtest/gtest.h>

#include "guild/guild.hpp"
#include "guild/guild_progression.hpp"
#include "raid/raid.hpp"
#include "tables/itemdat.h"

namespace devilution {
namespace {

class RaidProgressionTest : public testing::Test {
protected:
	void SetUp() override
	{
		ResetGuildProgression();
		GuildRewardDefinitions.clear();
	}
};

TEST_F(RaidProgressionTest, CompletionAppliesLockoutAndPreventsJoin)
{
	RaidInstanceState state {};
	state.raidId.value = 31337;
	state.difficulty = RaidDifficulty::Nightmare;
	state.phase = RaidPhase::InProgress;

	ASSERT_TRUE(CompleteRaid(state, 424242));
	EXPECT_EQ(state.phase, RaidPhase::Completed);
	EXPECT_EQ(state.lockoutState, RaidLockoutState::Active);
	EXPECT_EQ(state.lockoutExpirationTick, 424242u);

	const RaidMemberSnapshot member { .playerId = 1, .isAlive = true, .contribution = 0, .roleFlags = RaidRoleDamage };
	EXPECT_FALSE(CanJoinRaid(state, member, 0, 8));
}

TEST_F(RaidProgressionTest, RewardsAreDeterministicAndNotDoubleClaimed)
{
	GuildRewardDefinitions.push_back(GuildRewardDefinition {
	    .activity = GuildActivityType::DungeonClear,
	    .milestone = 1,
	    .rewardQuantity = 1,
	    .minGuildLevel = 1,
	    .rewardId = "raid-clear-1",
	});
	GuildRewardDefinitions.push_back(GuildRewardDefinition {
	    .activity = GuildActivityType::BossKill,
	    .milestone = 2,
	    .rewardQuantity = 1,
	    .minGuildLevel = 1,
	    .rewardId = "boss-kill-2",
	});

	const GuildId guildId { 7 };
	HandleRaidCompletionForGuild(1000, guildId, 5, 2);
	const auto firstRewards = GetGrantedGuildRewards();
	ASSERT_EQ(firstRewards.size(), 2u);

	HandleRaidCompletionForGuild(1000, guildId, 5, 2);
	EXPECT_EQ(GetGrantedGuildRewards().size(), 2u) << "Same raid completion must not grant duplicate claims";

	HandleRaidCompletionForGuild(1001, guildId, 5, 2);
	EXPECT_EQ(GetGrantedGuildRewards().size(), 4u) << "New raid id should be a distinct reward claim context";
}

TEST_F(RaidProgressionTest, LockoutStateCanBeResetForFreshCycle)
{
	RaidInstanceState state {};
	state.raidId.value = 123;
	state.difficulty = RaidDifficulty::Normal;
	state.phase = RaidPhase::InProgress;
	ASSERT_TRUE(FailRaid(state, 777));
	ASSERT_EQ(state.lockoutState, RaidLockoutState::Active);

	ResetRaid(state, 999);
	EXPECT_EQ(state.phase, RaidPhase::Inactive);
	EXPECT_EQ(state.lockoutState, RaidLockoutState::None);
	EXPECT_EQ(state.instanceSeed, 999u);
	EXPECT_EQ(state.objectiveBits, 0u);
}

} // namespace
} // namespace devilution
