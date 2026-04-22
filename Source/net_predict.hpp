#pragma once

#include "dvlnet/snapshot.hpp"

namespace devilution {

inline void NetRunPrediction(Player &player, int ticks = NET_PREDICTION_TICKS)
{
	(void)ticks;
	const uint8_t playerId = NetPlayerId(player);
	if (playerId >= MAX_PLRS)
		return;
	GetNetRuntimeState(playerId).predictedPosition = player.position.future;
}

inline void NetReconcile(Player &player, Point authoritativePosition)
{
	const uint8_t playerId = NetPlayerId(player);
	if (playerId < MAX_PLRS) {
		NetRuntimePlayerState &state = GetNetRuntimeState(playerId);
		state.predictedPosition = authoritativePosition;
		state.previousNetPosition = player.position.tile;
		state.nextNetPosition = authoritativePosition;
	}
	player.position.tile = authoritativePosition;
	player.position.future = authoritativePosition;
}

inline void NetUpdatePredictionAndHistory(Player &player)
{
	NetRunPrediction(player, NET_PREDICTION_TICKS);
	RecordLagHistory(player);
}

} // namespace devilution
