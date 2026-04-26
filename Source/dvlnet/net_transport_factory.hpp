#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "dvlnet/net_transport.hpp"
#include "dvlnet/net_transport_mode.hpp"
#include "dvlnet/sim_transport.hpp"
#include "dvlnet/udp_transport.hpp"

namespace devilution {

class UnsupportedTransport final : public INetTransport {
public:
	UnsupportedTransport(std::string_view name, std::string_view reason = "transport backend not implemented yet")
	    : name_(name)
	    , reason_(reason)
	{
	}

	tl::expected<void, std::string> Open(std::string_view /*bindAddress*/, uint16_t /*port*/) override
	{
		return tl::unexpected(reason_);
	}

	void Close() override {}
	[[nodiscard]] bool IsOpen() const override { return false; }

	tl::expected<std::size_t, std::string> Send(NetPacket /*packet*/) override
	{
		return tl::unexpected(reason_);
	}

	tl::expected<std::size_t, std::string> PollReceive(std::span<uint8_t> /*destination*/) override
	{
		return tl::unexpected(reason_);
	}

	[[nodiscard]] std::string_view Name() const override
	{
		return name_;
	}

private:
	std::string name_;
	std::string reason_;
};

struct NetTransportRuntimeConfig {
	NetTransport mode = NetTransport::Udp;
	bool enableChaos = false;
	uint32_t chaosSeed = 1337;
	NetChaosProfile chaosProfile {};
};

[[nodiscard]] inline std::unique_ptr<INetTransport> CreateNetTransport(const NetTransportRuntimeConfig &config)
{
	switch (config.mode) {
	case NetTransport::Simulation: {
		auto transport = std::make_unique<SimulatedLoopbackTransport>();
		if (config.enableChaos) {
			transport->SetChaosProfile(config.chaosSeed, config.chaosProfile);
		}
		return transport;
	}
	case NetTransport::Udp:
		return std::make_unique<UdpTransport>();
	case NetTransport::Quic:
		return std::make_unique<UnsupportedTransport>("quic", "QUIC transport is experimental and not available yet. Select UDP transport for multiplayer.");
	default:
		return std::make_unique<UnsupportedTransport>("unknown", "Unsupported network transport mode.");
	}
}

[[nodiscard]] inline std::unique_ptr<INetTransport> CreateNetTransport(NetTransport mode)
{
	return CreateNetTransport(NetTransportRuntimeConfig { .mode = mode });
}

} // namespace devilution
