#include <gtest/gtest.h>

#include "dvlnet/packet.h"

namespace devilution::net {
namespace {

#ifdef PACKET_ENCRYPTION

TEST(PacketEncryptionSaltTest, SameSaltAllowsEncryptedRoundTrip)
{
	const salt_t salt = packet_factory::GenerateSalt();
	buffer_t payload { 1, 2, 3, 5, 8 };

	packet_factory sender("secret-password", salt);
	auto outgoing = sender.make_packet<PT_MESSAGE>(1, 2, payload);
	ASSERT_TRUE(outgoing.has_value()) << outgoing.error().what();

	packet_factory receiver("secret-password", salt);
	auto incoming = receiver.make_packet((*outgoing)->Data());
	ASSERT_TRUE(incoming.has_value()) << incoming.error().what();
	EXPECT_EQ((*incoming)->Type(), PT_MESSAGE);
	EXPECT_EQ((*incoming)->Source(), 1);
	EXPECT_EQ((*incoming)->Destination(), 2);

	auto message = (*incoming)->Message();
	ASSERT_TRUE(message.has_value()) << message.error().what();
	EXPECT_EQ(**message, payload);
}

TEST(PacketEncryptionSaltTest, DifferentSaltRejectsEncryptedPacket)
{
	const salt_t senderSalt = packet_factory::GenerateSalt();
	salt_t receiverSalt = packet_factory::GenerateSalt();
	if (receiverSalt == senderSalt)
		receiverSalt = packet_factory::GenerateSalt();

	packet_factory sender("secret-password", senderSalt);
	auto outgoing = sender.make_packet<PT_MESSAGE>(1, 2, buffer_t { 13, 21, 34 });
	ASSERT_TRUE(outgoing.has_value()) << outgoing.error().what();

	packet_factory receiver("secret-password", receiverSalt);
	auto incoming = receiver.make_packet((*outgoing)->Data());
	ASSERT_FALSE(incoming.has_value());
	EXPECT_EQ(incoming.error().code(), PacketError::ErrorCode::DecryptionFailed);
}

#else

TEST(PacketEncryptionSaltTest, SaltApiCompilesWithoutPacketEncryption)
{
	packet_factory factory("secret-password", salt_t {});
	auto packet = factory.make_packet<PT_MESSAGE>(1, 2, buffer_t { 1, 2, 3 });
	ASSERT_TRUE(packet.has_value()) << packet.error().what();
	EXPECT_EQ((*packet)->Type(), PT_MESSAGE);
}

#endif

} // namespace
} // namespace devilution::net
