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
			player.guildMemberState = {};
			player.plractive = false;
		}
	}
};

TEST_F(GuildTest, InviteDoesNotCountAsAcceptedMembership)
{
	ASSERT_TRUE(CreateGuild(0, "Raiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	const GuildHallState hall = GetGuildHallState();
	EXPECT_EQ(hall.memberCount, 1u);

	const GuildMemberState invited = GetGuildMemberState(1);
	EXPECT_TRUE(invited.guildId.IsValid());
	EXPECT_TRUE(invited.invited);
	EXPECT_EQ(invited.role, MemberRole::None);
	EXPECT_TRUE(HasGuildInvite(1));
}

TEST_F(GuildTest, InvitedPlayerCanJoinGuild)
{
	ASSERT_TRUE(CreateGuild(0, "Raiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	EXPECT_TRUE(JoinGuild(1));
	EXPECT_FALSE(HasGuildInvite(1));

	const GuildMemberState member = GetGuildMemberState(1);
	EXPECT_EQ(member.role, MemberRole::Member);
	EXPECT_FALSE(member.invited);
	EXPECT_EQ(member.permissions, GuildPermissionNone);
	EXPECT_EQ(GetGuildHallState().memberCount, 2u);
}

TEST_F(GuildTest, CannotCreateGuildWhileInviteIsPending)
{
	ASSERT_TRUE(CreateGuild(0, "Raiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	EXPECT_FALSE(CreateGuild(1, "InvitedAlt"));
}

TEST_F(GuildTest, LeaderLeavingIgnoresPendingInvitesForOwnershipTransfer)
{
	ASSERT_TRUE(CreateGuild(0, "Raiders"));
	ASSERT_TRUE(InviteToGuild(0, 1));

	EXPECT_TRUE(LeaveGuild(0));
	EXPECT_FALSE(GetGuildHallState().guildId.IsValid());
	EXPECT_TRUE(HasGuildInvite(1));
	EXPECT_FALSE(JoinGuild(1));
}

} // namespace
} // namespace devilution
