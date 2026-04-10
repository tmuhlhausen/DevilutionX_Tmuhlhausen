#include "dvlnet/net_transport_factory.hpp"

#include <array>

#include <gtest/gtest.h>

namespace devilution {

TEST(NetTransportFactoryTest, SimulationTransportRoundTripsPackets)
{
	auto transport = CreateNetTransport(NetTransport::Simulation);
	ASSERT_TRUE(transport != nullptr);
	EXPECT_EQ(transport->Name(), "sim-loopback");
	EXPECT_FALSE(transport->IsOpen());

	ASSERT_TRUE(transport->Open("127.0.0.1", 6112).has_value());
	EXPECT_TRUE(transport->IsOpen());

	constexpr std::array<uint8_t, 4> Packet { 1, 2, 3, 4 };
	ASSERT_TRUE(transport->Send(NetPacket { Packet }).has_value());

	std::array<uint8_t, 8> readBuffer {};
	const auto bytes = transport->PollReceive(readBuffer);
	ASSERT_TRUE(bytes.has_value());
	EXPECT_EQ(*bytes, Packet.size());
	EXPECT_EQ(readBuffer[0], 1);
	EXPECT_EQ(readBuffer[1], 2);
	EXPECT_EQ(readBuffer[2], 3);
	EXPECT_EQ(readBuffer[3], 4);

	transport->Close();
	EXPECT_FALSE(transport->IsOpen());
}

TEST(NetTransportFactoryTest, UnsupportedTransportRejectsOpen)
{
	auto transport = CreateNetTransport(NetTransport::Udp);
	ASSERT_TRUE(transport != nullptr);
	EXPECT_EQ(transport->Name(), "udp");
	EXPECT_FALSE(transport->Open("0.0.0.0", 6112).has_value());
}

TEST(NetTransportFactoryTest, SimulationTransportSupportsChaosProfiles)
{
	NetTransportRuntimeConfig config {
		.mode = NetTransport::Simulation,
		.enableChaos = true,
		.chaosSeed = 42,
		.chaosProfile = NetChaosProfile { .dropRate = 1.0F, .duplicateRate = 0.0F, .reorderWindow = 1 },
	};
	auto transport = CreateNetTransport(config);
	auto *simTransport = dynamic_cast<SimulatedLoopbackTransport *>(transport.get());
	ASSERT_NE(simTransport, nullptr);
	ASSERT_TRUE(simTransport->Open("127.0.0.1", 6112).has_value());

	constexpr std::array<uint8_t, 2> Packet { 4, 2 };
	ASSERT_TRUE(simTransport->Send(NetPacket { Packet }).has_value());
	std::array<uint8_t, 8> readBuffer {};
	const auto bytes = simTransport->PollReceive(readBuffer);
	ASSERT_TRUE(bytes.has_value());
	EXPECT_EQ(*bytes, 0u);
}

} // namespace devilution
