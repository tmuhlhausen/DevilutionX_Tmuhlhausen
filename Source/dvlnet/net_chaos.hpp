#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <random>
#include <vector>

#include "dvlnet/net_transport.hpp"

namespace devilution {

struct NetChaosProfile {
	float dropRate = 0.0F;
	float duplicateRate = 0.0F;
	uint8_t reorderWindow = 0;
};

class NetChaosInjector {
public:
	explicit NetChaosInjector(uint32_t seed, NetChaosProfile profile)
	    : profile_(profile)
	    , rng_(seed)
	    , distribution_(0.0F, 1.0F)
	{
	}

	[[nodiscard]] std::vector<std::vector<uint8_t>> Process(NetPacket packet)
	{
		std::vector<std::vector<uint8_t>> output;
		if (ShouldDrop())
			return output;

		queued_.emplace_back(packet.data.begin(), packet.data.end());
		if (ShouldDuplicate())
			queued_.push_back(queued_.back());

		const size_t burst = std::max<size_t>(1, profile_.reorderWindow);
		while (!queued_.empty() && output.size() < burst) {
			if (profile_.reorderWindow > 1 && queued_.size() > 1 && Draw() < 0.5F) {
				output.push_back(std::move(queued_.back()));
				queued_.pop_back();
			} else {
				output.push_back(std::move(queued_.front()));
				queued_.pop_front();
			}
		}
		return output;
	}

	[[nodiscard]] size_t PendingPackets() const
	{
		return queued_.size();
	}

private:
	[[nodiscard]] float Draw()
	{
		return distribution_(rng_);
	}

	[[nodiscard]] bool ShouldDrop()
	{
		return profile_.dropRate > 0.0F && Draw() < profile_.dropRate;
	}

	[[nodiscard]] bool ShouldDuplicate()
	{
		return profile_.duplicateRate > 0.0F && Draw() < profile_.duplicateRate;
	}

	NetChaosProfile profile_;
	std::mt19937 rng_;
	std::uniform_real_distribution<float> distribution_;
	std::deque<std::vector<uint8_t>> queued_;
};

} // namespace devilution
