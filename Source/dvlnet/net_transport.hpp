#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include <expected.hpp>

namespace devilution {

struct NetPacket {
	std::span<const uint8_t> data;
};

class INetTransport {
public:
	virtual ~INetTransport() = default;

	virtual tl::expected<void, std::string> Open(std::string_view bindAddress, uint16_t port) = 0;
	virtual void Close() = 0;
	virtual bool IsOpen() const = 0;
	virtual tl::expected<std::size_t, std::string> Send(NetPacket packet) = 0;
	virtual tl::expected<std::size_t, std::string> PollReceive(std::span<uint8_t> destination) = 0;
	virtual std::string_view Name() const = 0;
};

} // namespace devilution
