#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#ifdef USE_SDL3
#include <SDL3/SDL_timer.h>
#else
#include <SDL.h>
#endif

#include "dvlnet/snapshot.hpp"

namespace devilution {

constexpr uint16_t NetMaxViolationScore = 100;
constexpr uint32_t NetActionRateWindowMs = 250;
constexpr uint8_t NetActionRateLimit = 8;

struct NetGuardPlayerState {
	uint16_t violationScore = 0;
	std::array<uint32_t, 8> lastActionTick {};
	std::array<uint8_t, 8> actionCount {};
	bool flagged = false;
};

inline std::array<NetGuardPlayerState, MAX_PLRS> NetGuardState {};

inline void NetApplyViolation(uint8_t playerId, uint8_t severity)
{
	if (playerId >= MAX_PLRS)
		return;

	NetGuardPlayerState &state = NetGuardState[playerId];
	state.violationScore = static_cast<uint16_t>(std::min<int>(NetMaxViolationScore, state.violationScore + severity));
	state.flagged = state.violationScore >= NetMaxViolationScore / 2;
}

[[nodiscard]] inline bool NetShouldDropPlayer(uint8_t playerId)
{
	return playerId < MAX_PLRS && NetGuardState[playerId].violationScore >= NetMaxViolationScore;
}

inline bool NetValidateMovement(uint8_t playerId, Point newPosition, int maxStepDistance = 2)
{
	if (playerId >= Players.size() || playerId >= MAX_PLRS)
		return false;

	const int distance = Players[playerId].position.tile.WalkingDistance(newPosition);
	if (distance > maxStepDistance) {
		NetApplyViolation(playerId, 25);
		return false;
	}
	return true;
}

inline bool NetValidateActionRate(uint8_t playerId, uint8_t actionType)
{
	if (playerId >= MAX_PLRS || actionType >= NetGuardState[playerId].lastActionTick.size())
		return false;

	NetGuardPlayerState &state = NetGuardState[playerId];
	const uint32_t now = SDL_GetTicks();
	const uint32_t last = state.lastActionTick[actionType];
	if (last == 0 || now - last > NetActionRateWindowMs) {
		state.actionCount[actionType] = 1;
		state.lastActionTick[actionType] = now;
		return true;
	}

	state.actionCount[actionType]++;
	state.lastActionTick[actionType] = now;
	if (state.actionCount[actionType] > NetActionRateLimit) {
		NetApplyViolation(playerId, 15);
		return false;
	}
	return true;
}

inline bool NetCheckStateInvariants(uint8_t playerId)
{
	if (playerId >= Players.size() || playerId >= MAX_PLRS)
		return false;

	const Player &player = Players[playerId];
	const bool valid = player._pStrength <= 750 && player._pMagic <= 750 && player._pDexterity <= 750 && player._pVitality <= 750 && player._pHitPoints <= player._pMaxHP;
	if (!valid)
		NetApplyViolation(playerId, 30);
	return valid;
}

} // namespace devilution
