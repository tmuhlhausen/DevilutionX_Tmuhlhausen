#include "dvlnet/net_transport_factory.hpp"

#include <array>
#include <chrono>
#include <thread>

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

TEST(NetTransportFactoryTest, UdpTransportBindsLoopbackAndRoundTripsPackets)
{
	auto transport = CreateNetTransport(NetTransport::Udp);
	ASSERT_TRUE(transport != nullptr);
	EXPECT_EQ(transport->Name(), "udp");
	EXPECT_FALSE(transport->IsOpen());

	ASSERT_TRUE(transport->Open("127.0.0.1", 0).has_value());
	EXPECT_TRUE(transport->IsOpen());

	constexpr std::array<uint8_t, 5> Packet { 8, 6, 7, 5, 3 };
	auto sent = transport->Send(NetPacket { Packet });
	ASSERT_TRUE(sent.has_value());
	EXPECT_EQ(*sent, Packet.size());

	std::array<uint8_t, 8> readBuffer {};
	std::size_t received = 0;
	for (int i = 0; i < 25; ++i) {
		auto bytes = transport->PollReceive(readBuffer);
		ASSERT_TRUE(bytes.has_value());
		received = *bytes;
		if (received == Packet.size())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	EXPECT_EQ(received, Packet.size());
	EXPECT_EQ(readBuffer[0], 8);
	EXPECT_EQ(readBuffer[1], 6);
	EXPECT_EQ(readBuffer[2], 7);
	EXPECT_EQ(readBuffer[3], 5);
	EXPECT_EQ(readBuffer[4], 3);

	transport->Close();
	EXPECT_FALSE(transport->IsOpen());
}

TEST(NetTransportFactoryTest, QuicTransportStaysExperimentalWithCapabilityError)
{
	auto transport = CreateNetTransport(NetTransport::Quic);
	ASSERT_TRUE(transport != nullptr);
	EXPECT_EQ(transport->Name(), "quic");

	const auto openResult = transport->Open("127.0.0.1", 6112);
	ASSERT_FALSE(openResult.has_value());
	EXPECT_EQ(openResult.error(), "QUIC transport is experimental and not available yet. Select UDP transport for multiplayer.");
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
