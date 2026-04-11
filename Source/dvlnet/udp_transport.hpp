#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <expected.hpp>

#include "dvlnet/net_transport.hpp"

namespace devilution {

class UdpTransport final : public INetTransport {
public:
	UdpTransport();
	~UdpTransport() override;

	tl::expected<void, std::string> Open(std::string_view bindAddress, uint16_t port) override;
	void Close() override;
	[[nodiscard]] bool IsOpen() const override;
	tl::expected<std::size_t, std::string> Send(NetPacket packet) override;
	tl::expected<std::size_t, std::string> PollReceive(std::span<uint8_t> destination) override;
	[[nodiscard]] std::string_view Name() const override;

private:
	struct Impl;
	Impl *impl_;
};

} // namespace devilution
