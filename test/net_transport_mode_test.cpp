#include "dvlnet/net_transport_mode.hpp"

#include <gtest/gtest.h>

namespace devilution {

TEST(NetTransportModeTest, ParsesSupportedModes)
{
	EXPECT_TRUE(ParseNetTransportMode("udp").has_value());
	EXPECT_TRUE(ParseNetTransportMode("quic").has_value());
	EXPECT_TRUE(ParseNetTransportMode("sim").has_value());
	EXPECT_EQ(*ParseNetTransportMode("udp"), NetTransport::Udp);
	EXPECT_EQ(*ParseNetTransportMode("quic"), NetTransport::Quic);
	EXPECT_EQ(*ParseNetTransportMode("sim"), NetTransport::Simulation);
}

TEST(NetTransportModeTest, RejectsUnsupportedModes)
{
	EXPECT_FALSE(ParseNetTransportMode("UDP").has_value());
	EXPECT_FALSE(ParseNetTransportMode("tcp").has_value());
	EXPECT_FALSE(ParseNetTransportMode("").has_value());
}

TEST(NetTransportModeTest, SerializesModes)
{
	EXPECT_EQ(NetTransportModeToString(NetTransport::Udp), "udp");
	EXPECT_EQ(NetTransportModeToString(NetTransport::Quic), "quic");
	EXPECT_EQ(NetTransportModeToString(NetTransport::Simulation), "sim");
}

} // namespace devilution
