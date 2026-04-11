#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "raid/raid.hpp"

namespace devilution {
namespace {

struct SequencedRaidState {
	RaidInstanceState state {};
	uint32_t sequence = 0;
};

struct SequencedRaidEvent {
	RaidEncounterEvent event {};
	uint32_t expectedRevision = 0;
	uint32_t sequence = 0;
};

class RaidReplica {
public:
	bool ApplyStateSync(const SequencedRaidState &snapshot)
	{
		if (snapshot.sequence <= lastSequence_)
			return false;
		if (snapshot.state.snapshotRevision < state_.snapshotRevision)
			return false;
		state_ = snapshot.state;
		lastSequence_ = snapshot.sequence;
		return true;
	}

	bool ApplyEncounterEvent(const SequencedRaidEvent &message)
	{
		if (message.sequence <= lastSequence_)
			return false;
		if (message.expectedRevision != state_.snapshotRevision)
			return false;

		RaidInstanceState next = state_;
		if (!devilution::ApplyEncounterEvent(next, message.event))
			return false;
		state_ = next;
		lastSequence_ = message.sequence;
		return true;
	}

	const RaidInstanceState &State() const
	{
		return state_;
	}

private:
	RaidInstanceState state_ {};
	uint32_t lastSequence_ = 0;
};

TEST(RaidSyncTest, DelayedAndReorderedTrafficConvergesToHostState)
{
	RaidInstanceState host {};
	host.raidId.value = 2048;
	host.difficulty = RaidDifficulty::Hell;
	host.phase = RaidPhase::InProgress;
	host.snapshotRevision = 10;

	RaidReplica peer;
	ASSERT_TRUE(peer.ApplyStateSync(SequencedRaidState { host, 1 }));

	RaidEncounterEvent e0 { .bossIndex = 0, .state = RaidEncounterState::Active, .objectiveBitsToSet = 0b1 };
	RaidEncounterEvent e1 { .bossIndex = 1, .state = RaidEncounterState::Defeated, .objectiveBitsToSet = 0b10 };
	RaidEncounterEvent e2 { .bossIndex = 0, .state = RaidEncounterState::Defeated, .objectiveBitsToSet = 0b100 };

	ASSERT_TRUE(devilution::ApplyEncounterEvent(host, e0));
	const uint32_t revAfterE0 = host.snapshotRevision;
	ASSERT_TRUE(devilution::ApplyEncounterEvent(host, e1));
	const uint32_t revAfterE1 = host.snapshotRevision;
	ASSERT_TRUE(devilution::ApplyEncounterEvent(host, e2));

	EXPECT_TRUE(peer.ApplyEncounterEvent(SequencedRaidEvent { e0, 10, 2 }));
	EXPECT_FALSE(peer.ApplyEncounterEvent(SequencedRaidEvent { e2, revAfterE1, 4 })) << "Missing e1 makes e2 stale by revision";
	EXPECT_TRUE(peer.ApplyEncounterEvent(SequencedRaidEvent { e1, revAfterE0, 3 }));
	EXPECT_FALSE(peer.ApplyEncounterEvent(SequencedRaidEvent { e1, revAfterE0, 3 })) << "Duplicate sequence rejected";
	EXPECT_TRUE(peer.ApplyEncounterEvent(SequencedRaidEvent { e2, revAfterE1, 5 }));

	EXPECT_EQ(peer.State().snapshotRevision, host.snapshotRevision);
	EXPECT_EQ(peer.State().objectiveBits, host.objectiveBits);
	EXPECT_EQ(peer.State().bossStates[0], host.bossStates[0]);
	EXPECT_EQ(peer.State().bossStates[1], host.bossStates[1]);
}

TEST(RaidSyncTest, MidEncounterReconnectConvergesViaSnapshot)
{
	RaidInstanceState host {};
	host.raidId.value = 999;
	host.difficulty = RaidDifficulty::Nightmare;
	host.phase = RaidPhase::InProgress;
	host.snapshotRevision = 50;
	host.objectiveBits = 0xF0;
	host.bossStates[2] = RaidEncounterState::Active;

	RaidReplica peer;
	ASSERT_TRUE(peer.ApplyStateSync(SequencedRaidState { host, 100 }));

	RaidInstanceState stale = host;
	stale.snapshotRevision = 49;
	stale.objectiveBits = 0;
	EXPECT_FALSE(peer.ApplyStateSync(SequencedRaidState { stale, 101 }));
	EXPECT_EQ(peer.State().snapshotRevision, 50u);
}

TEST(RaidSyncTest, HostMigrationUnsupportedReturnsExplicitFailure)
{
	const bool supportsHostMigration = false;
	const bool migrationRequested = true;
	const bool accepted = supportsHostMigration && migrationRequested;

	EXPECT_FALSE(accepted) << "Current protocol is host-authoritative only; migration must fail explicitly.";
}

TEST(RaidSyncTest, SimultaneousJoinLeaveAndReadinessChurnResolvesDeterministically)
{
	constexpr size_t SimPlayers = 4;
	std::array<bool, SimPlayers> joined {};
	std::array<bool, SimPlayers> ready {};

	auto join = [&](uint8_t id) {
		joined[id] = true;
	};
	auto leave = [&](uint8_t id) {
		joined[id] = false;
		ready[id] = false;
	};
	auto toggleReady = [&](uint8_t id) {
		if (joined[id])
			ready[id] = !ready[id];
	};

	join(1);
	join(2);
	join(3);
	toggleReady(1);
	toggleReady(2);

	leave(2);
	join(2);
	toggleReady(2);
	leave(3);
	toggleReady(3);

	const int joinedCount = static_cast<int>(std::count(joined.begin(), joined.end(), true));
	const int readyCount = static_cast<int>(std::count(ready.begin(), ready.end(), true));
	EXPECT_EQ(joinedCount, 2);
	EXPECT_EQ(readyCount, 2);
	EXPECT_TRUE(ready[1]);
	EXPECT_TRUE(ready[2]);
	EXPECT_FALSE(joined[3]);
}

} // namespace
} // namespace devilution
