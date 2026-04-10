#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "dvlnet/net_transport.hpp"
#include "dvlnet/net_transport_mode.hpp"
#include "dvlnet/sim_transport.hpp"

namespace devilution {

class UnsupportedTransport final : public INetTransport {
public:
	explicit UnsupportedTransport(std::string_view name)
	    : name_(name)
	{
	}

	tl::expected<void, std::string> Open(std::string_view /*bindAddress*/, uint16_t /*port*/) override
	{
		return tl::unexpected("transport backend not implemented yet");
	}

	void Close() override {}
	[[nodiscard]] bool IsOpen() const override { return false; }

	tl::expected<std::size_t, std::string> Send(NetPacket /*packet*/) override
	{
		return tl::unexpected("transport backend not implemented yet");
	}

	tl::expected<std::size_t, std::string> PollReceive(std::span<uint8_t> /*destination*/) override
	{
		return tl::unexpected("transport backend not implemented yet");
	}

	[[nodiscard]] std::string_view Name() const override
	{
		return name_;
	}

private:
	std::string name_;
};

[[nodiscard]] inline std::unique_ptr<INetTransport> CreateNetTransport(NetTransport mode)
{
	switch (mode) {
	case NetTransport::Simulation:
		return std::make_unique<SimulatedLoopbackTransport>();
	case NetTransport::Quic:
		return std::make_unique<UnsupportedTransport>("quic");
	case NetTransport::Udp:
		return std::make_unique<UnsupportedTransport>("udp");
	}
	return std::make_unique<UnsupportedTransport>("udp");
}

} // namespace devilution
