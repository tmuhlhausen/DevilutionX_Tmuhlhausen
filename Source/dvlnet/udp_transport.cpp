#include "dvlnet/udp_transport.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace devilution {
namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle InvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle InvalidSocket = -1;
#endif

void CloseSocket(SocketHandle socket)
{
#ifdef _WIN32
	closesocket(socket);
#else
	close(socket);
#endif
}

std::string GetLastSocketError()
{
#ifdef _WIN32
	return "socket error " + std::to_string(WSAGetLastError());
#else
	return std::strerror(errno);
#endif
}

bool IsWouldBlockError()
{
#ifdef _WIN32
	const int error = WSAGetLastError();
	return error == WSAEWOULDBLOCK;
#else
	return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
}

bool SetNonBlocking(SocketHandle socket)
{
#ifdef _WIN32
	u_long mode = 1;
	return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
	const int flags = fcntl(socket, F_GETFL, 0);
	if (flags < 0)
		return false;
	return fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

class SocketApiLifetime {
public:
	bool Acquire()
	{
#ifdef _WIN32
		std::lock_guard<std::mutex> lock(mutex_);
		if (refCount_ == 0) {
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				return false;
		}
		++refCount_;
#endif
		return true;
	}

	void Release()
	{
#ifdef _WIN32
		std::lock_guard<std::mutex> lock(mutex_);
		if (refCount_ == 0)
			return;
		--refCount_;
		if (refCount_ == 0)
			WSACleanup();
#endif
	}

private:
#ifdef _WIN32
	std::mutex mutex_;
	int refCount_ = 0;
#endif
};

SocketApiLifetime GlobalSocketApiLifetime;

} // namespace

struct UdpTransport::Impl {
	SocketHandle socket = InvalidSocket;
	sockaddr_in loopbackDestination {};
	bool isOpen = false;
	bool socketApiAcquired = false;
};

UdpTransport::UdpTransport()
    : impl_(new Impl())
{
}

UdpTransport::~UdpTransport()
{
	Close();
	delete impl_;
}

tl::expected<void, std::string> UdpTransport::Open(std::string_view bindAddress, uint16_t port)
{
	Close();

	if (!GlobalSocketApiLifetime.Acquire())
		return tl::unexpected("failed to initialize UDP socket layer");
	impl_->socketApiAcquired = true;

	impl_->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (impl_->socket == InvalidSocket) {
		Close();
		return tl::unexpected("failed to create UDP socket: " + GetLastSocketError());
	}

	if (!SetNonBlocking(impl_->socket)) {
		const std::string error = "failed to configure UDP socket as non-blocking: " + GetLastSocketError();
		Close();
		return tl::unexpected(error);
	}

	sockaddr_in bindEndpoint {};
	bindEndpoint.sin_family = AF_INET;
	bindEndpoint.sin_port = htons(port);

	const std::string bindAddressString(bindAddress);
	if (bindAddressString.empty() || bindAddressString == "0.0.0.0") {
		bindEndpoint.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	} else if (inet_pton(AF_INET, bindAddressString.c_str(), &bindEndpoint.sin_addr) != 1) {
		Close();
		return tl::unexpected("invalid IPv4 bind address: " + bindAddressString);
	}

	if (bind(impl_->socket, reinterpret_cast<const sockaddr *>(&bindEndpoint), sizeof(bindEndpoint)) != 0) {
		const std::string error = "failed to bind UDP socket: " + GetLastSocketError();
		Close();
		return tl::unexpected(error);
	}

	sockaddr_in localEndpoint {};
	socklen_t localEndpointLen = sizeof(localEndpoint);
	if (getsockname(impl_->socket, reinterpret_cast<sockaddr *>(&localEndpoint), &localEndpointLen) != 0) {
		const std::string error = "failed to resolve bound UDP socket endpoint: " + GetLastSocketError();
		Close();
		return tl::unexpected(error);
	}

	impl_->loopbackDestination = localEndpoint;
	impl_->isOpen = true;
	return {};
}

void UdpTransport::Close()
{
	if (impl_->socket != InvalidSocket) {
		CloseSocket(impl_->socket);
		impl_->socket = InvalidSocket;
	}
	if (impl_->socketApiAcquired) {
		GlobalSocketApiLifetime.Release();
		impl_->socketApiAcquired = false;
	}
	impl_->isOpen = false;
}

bool UdpTransport::IsOpen() const
{
	return impl_->isOpen;
}

tl::expected<std::size_t, std::string> UdpTransport::Send(NetPacket packet)
{
	if (!impl_->isOpen)
		return tl::unexpected("transport is closed");

	const int sent = sendto(impl_->socket, reinterpret_cast<const char *>(packet.data.data()), static_cast<int>(packet.data.size()), 0,
	    reinterpret_cast<const sockaddr *>(&impl_->loopbackDestination), sizeof(impl_->loopbackDestination));
	if (sent < 0)
		return tl::unexpected("failed to send UDP packet: " + GetLastSocketError());
	return static_cast<std::size_t>(sent);
}

tl::expected<std::size_t, std::string> UdpTransport::PollReceive(std::span<uint8_t> destination)
{
	if (!impl_->isOpen)
		return tl::unexpected("transport is closed");

	std::array<uint8_t, 2048> socketBuffer {};
	sockaddr_in peer {};
	socklen_t peerLength = sizeof(peer);
	const int received = recvfrom(impl_->socket, reinterpret_cast<char *>(socketBuffer.data()), static_cast<int>(socketBuffer.size()), 0,
	    reinterpret_cast<sockaddr *>(&peer), &peerLength);
	if (received < 0) {
		if (IsWouldBlockError())
			return static_cast<std::size_t>(0);
		return tl::unexpected("failed to receive UDP packet: " + GetLastSocketError());
	}
	const std::size_t bytesToCopy = std::min<std::size_t>(destination.size(), static_cast<std::size_t>(received));
	std::copy_n(socketBuffer.begin(), bytesToCopy, destination.begin());
	return bytesToCopy;
}

std::string_view UdpTransport::Name() const
{
	return "udp";
}

} // namespace devilution
