#include <gtest/gtest.h>

#include "msg.h"

namespace devilution {
namespace {

struct RaidProtocolState {
	uint32_t lastClientSequence = 0;
	uint32_t lastHostSequence = 0;
	uint32_t currentRaidId = 0;
	uint32_t currentRevision = 0;
};

bool ValidateActionPacket(const TCmdRaidAction &packet, RaidProtocolState &state)
{
	if (packet.payloadSize > MaxRaidActionPayload)
		return false;
	if (packet.actorPlayerId >= MAX_PLRS || packet.targetPlayerId >= MAX_PLRS)
		return false;
	const uint32_t sequence = Swap32LE(packet.sequence);
	if (sequence <= state.lastClientSequence)
		return false;
	if (packet.bCmd != CMD_RAID_CREATE && Swap32LE(packet.raidId) != state.currentRaidId)
		return false;
	if (Swap32LE(packet.expectedVersion) != state.currentRevision)
		return false;

	state.lastClientSequence = sequence;
	if (packet.bCmd == CMD_RAID_CREATE)
		state.currentRaidId = Swap32LE(packet.raidId);
	return true;
}

bool ValidateEventPacket(const TCmdRaidEvent &packet, RaidProtocolState &state)
{
	if (packet.payloadSize > MaxRaidEventPayload || packet.encounterIndex >= MaxRaidBosses)
		return false;
	const uint32_t sequence = Swap32LE(packet.sequence);
	if (sequence <= state.lastHostSequence)
		return false;
	if (Swap32LE(packet.raidId) != state.currentRaidId)
		return false;
	if (Swap32LE(packet.expectedVersion) != state.currentRevision)
		return false;

	state.lastHostSequence = sequence;
	return true;
}

TEST(RaidProtocolTest, RejectsMalformedPackets)
{
	RaidProtocolState state { .lastClientSequence = 0, .lastHostSequence = 0, .currentRaidId = 777, .currentRevision = 9 };
	TCmdRaidAction action {};
	action.bCmd = CMD_RAID_JOIN;
	action.actorPlayerId = 0;
	action.targetPlayerId = 0;
	action.payloadSize = MaxRaidActionPayload + 1;
	action.sequence = Swap32LE(1);
	action.raidId = Swap32LE(777);
	action.expectedVersion = Swap32LE(9);
	EXPECT_FALSE(ValidateActionPacket(action, state));

	action.payloadSize = 0;
	action.actorPlayerId = MAX_PLRS;
	EXPECT_FALSE(ValidateActionPacket(action, state));
}

TEST(RaidProtocolTest, RejectsStaleClientAndHostSequence)
{
	RaidProtocolState state { .lastClientSequence = 10, .lastHostSequence = 40, .currentRaidId = 888, .currentRevision = 2 };

	TCmdRaidAction action {};
	action.bCmd = CMD_RAID_READY_TOGGLE;
	action.actorPlayerId = 1;
	action.targetPlayerId = 1;
	action.sequence = Swap32LE(10);
	action.raidId = Swap32LE(888);
	action.expectedVersion = Swap32LE(2);
	EXPECT_FALSE(ValidateActionPacket(action, state));

	action.sequence = Swap32LE(11);
	EXPECT_TRUE(ValidateActionPacket(action, state));

	TCmdRaidEvent event {};
	event.bCmd = CMD_RAID_EVENT;
	event.sequence = Swap32LE(40);
	event.raidId = Swap32LE(888);
	event.expectedVersion = Swap32LE(2);
	event.encounterIndex = 0;
	EXPECT_FALSE(ValidateEventPacket(event, state));

	event.sequence = Swap32LE(41);
	EXPECT_TRUE(ValidateEventPacket(event, state));
}

TEST(RaidProtocolTest, MidEncounterReconnectAcceptsFreshSnapshotAndRejectsOlder)
{
	RaidProtocolState state { .lastClientSequence = 0, .lastHostSequence = 100, .currentRaidId = 9001, .currentRevision = 21 };

	TCmdRaidState staleSnapshot {};
	staleSnapshot.bCmd = CMD_RAID_STATE_SYNC;
	staleSnapshot.raidId = Swap32LE(9001);
	staleSnapshot.sequence = Swap32LE(99);
	staleSnapshot.snapshotRevision = Swap32LE(20);
	EXPECT_LE(Swap32LE(staleSnapshot.sequence), state.lastHostSequence);
	EXPECT_LT(Swap32LE(staleSnapshot.snapshotRevision), state.currentRevision);

	TCmdRaidState freshSnapshot = staleSnapshot;
	freshSnapshot.sequence = Swap32LE(101);
	freshSnapshot.snapshotRevision = Swap32LE(22);
	EXPECT_GT(Swap32LE(freshSnapshot.sequence), state.lastHostSequence);
	EXPECT_GT(Swap32LE(freshSnapshot.snapshotRevision), state.currentRevision);
}

} // namespace
} // namespace devilution
