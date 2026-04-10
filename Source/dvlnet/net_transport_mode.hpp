#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace devilution {

enum class NetTransport : uint8_t {
	Udp = 0,
	Quic = 1,
	Simulation = 2,
};

[[nodiscard]] inline std::optional<NetTransport> ParseNetTransportMode(std::string_view value)
{
	if (value == "udp")
		return NetTransport::Udp;
	if (value == "quic")
		return NetTransport::Quic;
	if (value == "sim")
		return NetTransport::Simulation;
	return std::nullopt;
}

[[nodiscard]] inline std::string_view NetTransportModeToString(NetTransport transport)
{
	switch (transport) {
	case NetTransport::Udp:
		return "udp";
	case NetTransport::Quic:
		return "quic";
	case NetTransport::Simulation:
		return "sim";
	}
	return "udp";
}

} // namespace devilution
