#include <gtest/gtest.h>

#include "guild/guild.hpp"
#include "player.h"

namespace devilution {
namespace {

class GuildTest : public testing::Test {
protected:
	void SetUp() override
	{
		ResetGuildState();
		for (Player &player : Players) {
			player.plractive = false;
			player.guildMemberState = {};
		}
	}
};

TEST_F(GuildTest, InviteDoesNotCountAsAcceptedMembership)
{
	ASSERT_TRUE(CreateGuild(0, "RoadmapRaiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	const GuildHallState hall = GetGuildHallState();
	EXPECT_EQ(hall.memberCount, 1);
	EXPECT_TRUE(HasGuildInvite(1));
	EXPECT_EQ(GetGuildMemberState(1).role, MemberRole::None);
	EXPECT_TRUE(GetGuildMemberState(1).invited);
}

TEST_F(GuildTest, InvitedPlayerCanJoinGuild)
{
	ASSERT_TRUE(CreateGuild(0, "RoadmapRaiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	EXPECT_TRUE(JoinGuild(1));
	EXPECT_FALSE(HasGuildInvite(1));
	EXPECT_EQ(GetGuildMemberState(1).role, MemberRole::Member);
	EXPECT_FALSE(GetGuildMemberState(1).invited);
	EXPECT_EQ(GetGuildHallState().memberCount, 2);
}

TEST_F(GuildTest, ResetClearsPlayerMirrorState)
{
	ASSERT_TRUE(CreateGuild(0, "RoadmapRaiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	ASSERT_TRUE(Players[0].guildMemberState.guildId.IsValid());
	ASSERT_TRUE(Players[1].guildMemberState.guildId.IsValid());

	ResetGuildState();

	EXPECT_FALSE(GetGuildMemberState(0).guildId.IsValid());
	EXPECT_FALSE(GetGuildMemberState(1).guildId.IsValid());
	EXPECT_FALSE(Players[0].guildMemberState.guildId.IsValid());
	EXPECT_FALSE(Players[1].guildMemberState.guildId.IsValid());
}

TEST_F(GuildTest, LeaderTransferIgnoresPendingInvites)
{
	ASSERT_TRUE(CreateGuild(0, "RoadmapRaiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));
	ASSERT_TRUE(InviteToGuild(0, 2));
	ASSERT_TRUE(JoinGuild(2));

	ASSERT_TRUE(LeaveGuild(0));

	EXPECT_TRUE(HasGuildInvite(1));
	EXPECT_EQ(GetGuildMemberState(1).role, MemberRole::None);
	EXPECT_EQ(GetGuildMemberState(2).role, MemberRole::Leader);
	EXPECT_EQ(Players[2].guildMemberState.role, MemberRole::Leader);
}

} // namespace
} // namespace devilution
