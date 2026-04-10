#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <expected.hpp>

#include "dvlnet/net_transport.hpp"
#include "dvlnet/net_chaos.hpp"

namespace devilution {

class SimulatedLoopbackTransport final : public INetTransport {
public:
	tl::expected<void, std::string> Open(std::string_view bindAddress, uint16_t port) override
	{
		bindAddress_ = std::string(bindAddress);
		port_ = port;
		isOpen_ = true;
		inbox_.clear();
		return {};
	}

	void Close() override
	{
		isOpen_ = false;
		inbox_.clear();
	}

	[[nodiscard]] bool IsOpen() const override
	{
		return isOpen_;
	}

	tl::expected<std::size_t, std::string> Send(NetPacket packet) override
	{
		if (!isOpen_)
			return tl::unexpected("transport is closed");
		if (chaos_.has_value()) {
			for (std::vector<uint8_t> &mutated : chaos_->Process(packet)) {
				inbox_.emplace_back(mutated.begin(), mutated.end());
			}
		} else {
			inbox_.emplace_back(packet.data.begin(), packet.data.end());
		}
		return packet.data.size();
	}

	tl::expected<std::size_t, std::string> PollReceive(std::span<uint8_t> destination) override
	{
		if (!isOpen_)
			return tl::unexpected("transport is closed");
		if (inbox_.empty())
			return static_cast<std::size_t>(0);

		std::vector<uint8_t> packet = std::move(inbox_.front());
		inbox_.pop_front();
		const std::size_t bytesToCopy = std::min(destination.size(), packet.size());
		std::copy_n(packet.begin(), bytesToCopy, destination.begin());
		return bytesToCopy;
	}

	[[nodiscard]] std::string_view Name() const override
	{
		return "sim-loopback";
	}

	void SetChaosProfile(uint32_t seed, NetChaosProfile profile)
	{
		chaos_.emplace(seed, profile);
	}

private:
	std::string bindAddress_;
	uint16_t port_ = 0;
	bool isOpen_ = false;
	std::optional<NetChaosInjector> chaos_;
	std::deque<std::vector<uint8_t>> inbox_;
};

} // namespace devilution
