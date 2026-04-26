/**
 * @file snapshot.hpp
 *
 * Lightweight 2026 multiplayer snapshot runtime.
 *
 * This layer is intentionally header-only so it can be wired into the current
 * command pipeline incrementally. It complements the existing rollback input
 * queue by tracking authoritative player snapshots, interpolation targets,
 * lag-compensation history, and stale-sequence rejection.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#ifdef USE_SDL3
#include <SDL3/SDL_timer.h>
#else
#include <SDL.h>
#endif

#include "multi.h"
#include "player.h"

namespace devilution {

constexpr size_t NetLagHistorySize = 32;

struct NetSnapshotPlayerState {
	uint8_t playerId = 0;
	Point tile {};
	Point future {};
	int32_t hitPoints = 0;
	int32_t mana = 0;
	uint8_t characterLevel = 1;
	uint8_t dungeonLevel = 0;
	uint8_t isSetLevel = 0;
};

struct NetSnapshot {
	uint32_t sequence = 0;
	uint64_t timestamp = 0;
	uint32_t activeMask = 0;
	std::array<NetSnapshotPlayerState, MAX_PLRS> players {};
};

struct NetPositionHistory {
	Point tile {};
	uint64_t timestamp = 0;
};

struct NetRuntimePlayerState {
	Point previousNetPosition {};
	Point nextNetPosition {};
	Point predictedPosition {};
	uint64_t lastSnapshotTimestamp = 0;
	uint32_t nextSequence = 1;
	uint32_t lastReceivedSequence = 0;
	float interpolationAlpha = 0.0F;
	std::array<NetPositionHistory, NetLagHistorySize> history {};
	uint8_t historyIndex = 0;
};

inline std::array<NetRuntimePlayerState, MAX_PLRS> NetPlayerRuntime {};

[[nodiscard]] inline uint8_t NetPlayerId(const Player &player)
{
	return player.getId();
}

[[nodiscard]] inline NetRuntimePlayerState &GetNetRuntimeState(uint8_t playerId)
{
	return NetPlayerRuntime[std::min<size_t>(playerId, MAX_PLRS - 1)];
}

[[nodiscard]] inline bool IsNewerNetSequence(uint32_t sequence, uint32_t lastReceived)
{
	return sequence != lastReceived && static_cast<int32_t>(sequence - lastReceived) > 0;
}

inline void RecordLagHistory(uint8_t playerId, const Player &player)
{
	if (playerId >= MAX_PLRS)
		return;

	NetRuntimePlayerState &state = NetPlayerRuntime[playerId];
	state.history[state.historyIndex] = { player.position.tile, SDL_GetTicks64() };
	state.historyIndex = static_cast<uint8_t>((state.historyIndex + 1) % state.history.size());
}

inline void RecordLagHistory(Player &player)
{
	RecordLagHistory(NetPlayerId(player), player);
}

[[nodiscard]] inline NetSnapshot CreateSnapshot()
{
	NetSnapshot snapshot {};
	snapshot.timestamp = SDL_GetTicks64();
	if (MyPlayer != nullptr && MyPlayerId < MAX_PLRS)
		snapshot.sequence = NetPlayerRuntime[MyPlayerId].nextSequence++;

	const size_t playerCount = std::min<size_t>(Players.size(), MAX_PLRS);
	for (size_t i = 0; i < playerCount; ++i) {
		const Player &player = Players[i];
		if (!player.plractive)
			continue;

		snapshot.activeMask |= 1U << i;
		NetSnapshotPlayerState &playerState = snapshot.players[i];
		playerState.playerId = static_cast<uint8_t>(i);
		playerState.tile = player.position.tile;
		playerState.future = player.position.future;
		playerState.hitPoints = player._pHitPoints;
		playerState.mana = player._pMana;
		playerState.characterLevel = player.getCharacterLevel();
		playerState.dungeonLevel = player.plrlevel;
		playerState.isSetLevel = player.plrIsOnSetLevel ? 1 : 0;
	}

	return snapshot;
}

inline void ApplySnapshot(const NetSnapshot &snapshot, bool reconcile)
{
	const size_t playerCount = std::min<size_t>(Players.size(), MAX_PLRS);
	for (size_t i = 0; i < playerCount; ++i) {
		if ((snapshot.activeMask & (1U << i)) == 0)
			continue;
		if (i == MyPlayerId && !reconcile)
			continue;

		NetRuntimePlayerState &state = NetPlayerRuntime[i];
		if (!IsNewerNetSequence(snapshot.sequence, state.lastReceivedSequence))
			continue;

		const NetSnapshotPlayerState &playerState = snapshot.players[i];
		Player &player = Players[i];
		state.previousNetPosition = player.position.tile;
		state.nextNetPosition = playerState.tile;
		state.lastSnapshotTimestamp = snapshot.timestamp;
		state.lastReceivedSequence = snapshot.sequence;

		player.position.tile = playerState.tile;
		player.position.future = playerState.future;
		player._pHitPoints = playerState.hitPoints;
		player._pMana = playerState.mana;
		player.plrlevel = playerState.dungeonLevel;
		player.plrIsOnSetLevel = playerState.isSetLevel != 0;
		player.setCharacterLevel(playerState.characterLevel);
	}
}

inline void InterpolateRemotePlayers()
{
	const uint64_t now = SDL_GetTicks64();
	const size_t playerCount = std::min<size_t>(Players.size(), MAX_PLRS);
	for (size_t i = 0; i < playerCount; ++i) {
		if (i == MyPlayerId)
			continue;

		Player &player = Players[i];
		NetRuntimePlayerState &state = NetPlayerRuntime[i];
		if (!player.plractive || state.lastSnapshotTimestamp == 0)
			continue;

		const uint64_t elapsed = now > state.lastSnapshotTimestamp ? now - state.lastSnapshotTimestamp : 0;
		state.interpolationAlpha = std::clamp(static_cast<float>(elapsed) / static_cast<float>(NET_SNAPSHOT_INTERVAL_MS), 0.0F, 1.0F);
		if (elapsed >= NET_SNAPSHOT_INTERVAL_MS)
			player.position.tile = state.nextNetPosition;
	}
}

[[nodiscard]] inline Point RewindPosition(uint8_t playerId, uint64_t clientSendTime, uint32_t roundTripMs)
{
	if (playerId >= MAX_PLRS || playerId >= Players.size())
		return {};

	const NetRuntimePlayerState &state = NetPlayerRuntime[playerId];
	const uint64_t halfRoundTrip = roundTripMs / 2;
	const uint64_t rewindTime = clientSendTime > halfRoundTrip ? clientSendTime - halfRoundTrip : 0;

	for (size_t offset = 0; offset < state.history.size(); ++offset) {
		const size_t index = (state.historyIndex + state.history.size() - 1 - offset) % state.history.size();
		const NetPositionHistory &entry = state.history[index];
		if (entry.timestamp != 0 && entry.timestamp <= rewindTime)
			return entry.tile;
	}

	return Players[playerId].position.tile;
}

} // namespace devilution
