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
	uint8_t latencyTicks = 0;
	uint8_t jitterTicks = 0;
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
		const uint64_t dueTick = currentTick_ + DelayTicks();
		queued_.push_back(QueuedPacket { .dueTick = dueTick, .payload = std::vector<uint8_t>(packet.data.begin(), packet.data.end()) });
		if (ShouldDuplicate())
			queued_.push_back(queued_.back());
		++currentTick_;

		const size_t burst = std::max<size_t>(1, profile_.reorderWindow);
		while (!queued_.empty() && output.size() < burst && queued_.front().dueTick <= currentTick_) {
			if (profile_.reorderWindow > 1 && queued_.size() > 1 && Draw() < 0.5F && queued_.back().dueTick <= currentTick_) {
				output.push_back(std::move(queued_.back().payload));
				queued_.pop_back();
			} else {
				output.push_back(std::move(queued_.front().payload));
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
	[[nodiscard]] uint8_t DelayTicks()
	{
		const uint8_t jitterSpan = profile_.jitterTicks;
		if (jitterSpan == 0)
			return profile_.latencyTicks;
		const float jitterDraw = Draw() * static_cast<float>(jitterSpan + 1);
		const uint8_t jitter = static_cast<uint8_t>(jitterDraw);
		return static_cast<uint8_t>(profile_.latencyTicks + jitter);
	}

	struct QueuedPacket {
		uint64_t dueTick = 0;
		std::vector<uint8_t> payload;
	};

	NetChaosProfile profile_;
	std::mt19937 rng_;
	std::uniform_real_distribution<float> distribution_;
	std::deque<QueuedPacket> queued_;
	uint64_t currentTick_ = 0;
};

} // namespace devilution
