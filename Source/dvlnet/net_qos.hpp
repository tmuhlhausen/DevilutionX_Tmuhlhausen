#pragma once

#include <algorithm>

namespace devilution {

struct NetLinkMetrics {
	float rttMs = 0.0F;
	float jitterMs = 0.0F;
	float lossPct = 0.0F;
	float bytesPerSecondCap = 0.0F;
};

class NetPacketBudgetController {
public:
	explicit NetPacketBudgetController(float baseBytesPerTick)
	    : baseBytesPerTick_(std::max(1.0F, baseBytesPerTick))
	    , smoothedBudget_(baseBytesPerTick_)
	{
	}

	void Update(const NetLinkMetrics &metrics)
	{
		const float safeLoss = std::clamp(metrics.lossPct, 0.0F, 95.0F);
		const float lossFactor = 1.0F - (safeLoss / 100.0F);
		const float latencyPenalty = 1.0F / (1.0F + ((metrics.rttMs + metrics.jitterMs) / 250.0F));
		float rawBudget = baseBytesPerTick_ * lossFactor * latencyPenalty;
		if (metrics.bytesPerSecondCap > 0.0F) {
			const float capPerTick = metrics.bytesPerSecondCap / 20.0F;
			rawBudget = std::min(rawBudget, capPerTick);
		}

		rawBudget = std::clamp(rawBudget, baseBytesPerTick_ * 0.1F, baseBytesPerTick_ * 1.2F);
		smoothedBudget_ = (smoothedBudget_ * 0.8F) + (rawBudget * 0.2F);
	}

	[[nodiscard]] int BudgetBytes() const
	{
		return static_cast<int>(smoothedBudget_);
	}

private:
	float baseBytesPerTick_;
	float smoothedBudget_;
};

} // namespace devilution
