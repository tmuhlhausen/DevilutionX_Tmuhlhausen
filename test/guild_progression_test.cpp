#include <gtest/gtest.h>

#include "guild/guild.hpp"
#include "guild/guild_progression.hpp"
#include "tables/itemdat.h"

namespace devilution {
namespace {

class GuildProgressionTest : public testing::Test {
protected:
	void SetUp() override
	{
		ResetGuildProgression();
		GuildRewardDefinitions.clear();
	}
};

TEST_F(GuildProgressionTest, SeasonTransitionResetsSeasonMilestonesAndAccruesPrestige)
{
	GuildRewardDefinitions.push_back(GuildRewardDefinition {
	    .activity = GuildActivityType::DungeonClear,
	    .milestone = 1,
	    .rewardQuantity = 10,
	    .minGuildLevel = 1,
	    .rewardId = "season-clear-1",
	});

	const GuildId guildId { 77 };
	HandleRaidCompletionForGuild(1, guildId, 5, 0);
	ASSERT_EQ(GetGrantedGuildRewards().size(), 1u);

	AdvanceGuildSeasonIfNeeded(4);
	GuildProgressionPersistedState state = GetGuildProgressionPersistedState();
	EXPECT_EQ(state.seasonId, 2u);
	EXPECT_EQ(state.seasonTransitions, 1u);
	EXPECT_GT(state.prestigePoints, 0u);

	HandleRaidCompletionForGuild(2, guildId, 5, 0);
	EXPECT_EQ(GetGrantedGuildRewards().size(), 2u) << "Season reset should allow season-limited rewards to be granted again.";
}

TEST_F(GuildProgressionTest, RankingSnapshotUsesAccumulatedActivityTier)
{
	const GuildId guildId { 88 };
	EmitGuildActivityEvent(guildId, GuildActivityType::BossKill, 120, 8, 99);
	EmitGuildActivityEvent(guildId, GuildActivityType::DungeonClear, 40, 8, 99);
	EXPECT_EQ(CalculateGuildSeasonTier(), 3u);

	CaptureGuildRankingSnapshot(12345);
	const GuildProgressionPersistedState state = GetGuildProgressionPersistedState();
	ASSERT_EQ(state.usedRankingSnapshots, 1u);
	EXPECT_EQ(state.rankingSnapshots[0].snapshotId, 12345u);
	EXPECT_EQ(state.rankingSnapshots[0].tier, 3u);
	EXPECT_EQ(state.rankingSnapshots[0].accumulatedActivity, 160u);
}

TEST_F(GuildProgressionTest, RewardClaimDedupBlocksSameSeasonClaimsAndAllowsNextSeason)
{
	GuildRewardDefinitions.push_back(GuildRewardDefinition {
	    .activity = GuildActivityType::BossKill,
	    .milestone = 2,
	    .rewardQuantity = 5,
	    .minGuildLevel = 1,
	    .rewardId = "boss-2",
	});

	const GuildId guildId { 11 };
	HandleRaidCompletionForGuild(500, guildId, 10, 2);
	ASSERT_EQ(GetGrantedGuildRewards().size(), 1u);
	const auto first = GetGrantedGuildRewards().front();
	EXPECT_EQ(first.rewardKind, GuildRewardKind::TokenCurrency);
	EXPECT_EQ(first.tokenCurrency, 5u);

	HandleRaidCompletionForGuild(500, guildId, 10, 2);
	EXPECT_EQ(GetGrantedGuildRewards().size(), 1u) << "Dedup key should block duplicate season claims.";

	AdvanceGuildSeasonIfNeeded(4);
	HandleRaidCompletionForGuild(501, guildId, 10, 2);
	ASSERT_EQ(GetGrantedGuildRewards().size(), 2u);
	EXPECT_NE(GetGrantedGuildRewards()[0].claimDedupKey, GetGrantedGuildRewards()[1].claimDedupKey);
}

} // namespace
} // namespace devilution
