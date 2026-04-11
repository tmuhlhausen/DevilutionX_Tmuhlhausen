#pragma once

#include <cstdint>

#include "raid/raid_state.hpp"

namespace devilution {

[[nodiscard]] bool CanJoin(const RaidInstanceState &state, const RaidMemberSnapshot &member, uint8_t activeMemberCount, uint8_t maxMembers);
[[nodiscard]] bool CanStart(const RaidInstanceState &state, uint8_t readyMemberCount, uint8_t minimumMemberCount);
[[nodiscard]] bool CanProgressPhase(RaidPhase current, RaidPhase next);
[[nodiscard]] bool CanReset(const RaidInstanceState &state);

[[nodiscard]] bool CanJoinRaid(const RaidInstanceState &state, const RaidMemberSnapshot &member, uint8_t activeMemberCount, uint8_t maxMembers);
[[nodiscard]] bool CanStartRaid(const RaidInstanceState &state, uint8_t readyMemberCount, uint8_t minimumMemberCount);

} // namespace devilution
