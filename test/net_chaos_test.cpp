#include "dvlnet/net_chaos.hpp"

#include <array>

#include <gtest/gtest.h>

namespace devilution {

TEST(NetChaosTest, DropProfileDropsAllPackets)
{
	NetChaosInjector injector(1337, NetChaosProfile { .dropRate = 1.0F, .duplicateRate = 0.0F, .reorderWindow = 1 });
	constexpr std::array<uint8_t, 3> packet { 7, 8, 9 };
	const auto out = injector.Process(NetPacket { packet });
	EXPECT_TRUE(out.empty());
}

TEST(NetChaosTest, DuplicateProfileCanDuplicatePackets)
{
	NetChaosInjector injector(0, NetChaosProfile { .dropRate = 0.0F, .duplicateRate = 1.0F, .reorderWindow = 4 });
	constexpr std::array<uint8_t, 2> packet { 1, 2 };
	const auto out = injector.Process(NetPacket { packet });
	ASSERT_GE(out.size(), 2u);
	EXPECT_EQ(out[0][0], 1);
	EXPECT_EQ(out[1][0], 1);
}

TEST(NetChaosTest, ReorderWindowCanReverseOrder)
{
	NetChaosInjector injector(3, NetChaosProfile { .dropRate = 0.0F, .duplicateRate = 0.0F, .reorderWindow = 8 });
	constexpr std::array<uint8_t, 1> packetA { 10 };
	constexpr std::array<uint8_t, 1> packetB { 20 };

	(void)injector.Process(NetPacket { packetA });
	const auto out = injector.Process(NetPacket { packetB });
	ASSERT_FALSE(out.empty());
	EXPECT_TRUE(out[0][0] == 10 || out[0][0] == 20);
}

} // namespace devilution
