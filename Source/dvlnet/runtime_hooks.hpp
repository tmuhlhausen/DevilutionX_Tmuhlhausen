#pragma once

#include "dvlnet/snapshot.hpp"
#include "net_guard.hpp"
#include "net_predict.hpp"

namespace devilution {

inline bool NetOnRemoteWalkCommand(uint8_t playerId, Point newPosition)
{
	return NetValidateMovement(playerId, newPosition) && NetValidateActionRate(playerId, 0);
}

inline bool NetOnRemoteActionCommand(uint8_t playerId, uint8_t actionBucket)
{
	return NetValidateActionRate(playerId, static_cast<uint8_t>(1 + (actionBucket % 7))) && NetCheckStateInvariants(playerId) && !NetShouldDropPlayer(playerId);
}

inline void NetOnLocalSimulationTick()
{
	InterpolateRemotePlayers();
	const size_t playerCount = std::min<size_t>(Players.size(), MAX_PLRS);
	for (size_t i = 0; i < playerCount; ++i) {
		if (!Players[i].plractive)
			continue;
		NetUpdatePredictionAndHistory(Players[i]);
	}
}

inline NetSnapshot NetBuildAuthoritativeSnapshot()
{
	return CreateSnapshot();
}

inline void NetApplyAuthoritativeSnapshot(const NetSnapshot &snapshot, bool reconcile)
{
	ApplySnapshot(snapshot, reconcile);
}

} // namespace devilution
