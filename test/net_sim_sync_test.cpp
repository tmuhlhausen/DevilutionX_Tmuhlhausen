#include "dvlnet/net_chaos.hpp"

#include <array>
#include <vector>

#include <gtest/gtest.h>

namespace devilution {
namespace {

std::vector<uint8_t> Flatten(const std::vector<std::vector<uint8_t>> &packets)
{
	std::vector<uint8_t> values;
	for (const auto &packet : packets) {
		if (!packet.empty())
			values.push_back(packet.front());
	}
	return values;
}

} // namespace

TEST(NetSimSyncTest, DeterministicLatencyAndJitterStillPreserveSyncOrder)
{
	NetChaosInjector injector(19, NetChaosProfile { .dropRate = 0.0F, .duplicateRate = 0.0F, .reorderWindow = 1, .latencyTicks = 2, .jitterTicks = 1 });
	std::vector<uint8_t> observed;
	for (uint8_t tick = 1; tick <= 20; ++tick) {
		const std::array<uint8_t, 1> payload { tick };
		const auto out = injector.Process(NetPacket { payload });
		const std::vector<uint8_t> flattened = Flatten(out);
		observed.insert(observed.end(), flattened.begin(), flattened.end());
	}
	for (int i = 0; i < 6; ++i) {
		const std::array<uint8_t, 1> payload { 0 };
		const auto out = injector.Process(NetPacket { payload });
		const std::vector<uint8_t> flattened = Flatten(out);
		for (uint8_t value : flattened) {
			if (value != 0)
				observed.push_back(value);
		}
	}
	ASSERT_FALSE(observed.empty());
	for (size_t i = 1; i < observed.size(); ++i) {
		EXPECT_GE(observed[i], observed[i - 1]);
	}
}

TEST(NetSimSyncTest, DeterministicDropScenarioMatchesExpectedSyncDensity)
{
	NetChaosInjector injector(1234, NetChaosProfile { .dropRate = 0.35F, .duplicateRate = 0.0F, .reorderWindow = 1 });
	int delivered = 0;
	for (uint8_t tick = 1; tick <= 40; ++tick) {
		const std::array<uint8_t, 1> payload { tick };
		delivered += static_cast<int>(injector.Process(NetPacket { payload }).size());
	}
	EXPECT_GE(delivered, 20);
	EXPECT_LE(delivered, 35);
}

TEST(NetSimSyncTest, DeterministicReorderScenarioProducesOutOfOrderFrames)
{
	NetChaosInjector injector(7, NetChaosProfile { .dropRate = 0.0F, .duplicateRate = 0.0F, .reorderWindow = 6 });
	std::vector<uint8_t> observed;
	for (uint8_t tick = 1; tick <= 12; ++tick) {
		const std::array<uint8_t, 1> payload { tick };
		const auto out = injector.Process(NetPacket { payload });
		const std::vector<uint8_t> flattened = Flatten(out);
		observed.insert(observed.end(), flattened.begin(), flattened.end());
	}
	ASSERT_GE(observed.size(), 4u);
	bool hasReorder = false;
	for (size_t i = 1; i < observed.size(); ++i) {
		if (observed[i] < observed[i - 1]) {
			hasReorder = true;
			break;
		}
	}
	EXPECT_TRUE(hasReorder);
}

} // namespace devilution
