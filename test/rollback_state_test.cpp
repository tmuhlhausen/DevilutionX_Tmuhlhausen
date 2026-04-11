#include "dvlnet/rollback_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace devilution::dvlnet {
namespace {

std::vector<std::byte> ToBytes(std::initializer_list<uint8_t> values)
{
	std::vector<std::byte> result;
	result.reserve(values.size());
	for (const uint8_t value : values)
		result.push_back(static_cast<std::byte>(value));
	return result;
}

} // namespace

TEST(RollbackStateTest, SnapshotRestoreDeterminism)
{
	RollbackState rollback(8);
	const std::vector<std::byte> snapshot = ToBytes({ 1, 2, 3, 4, 5 });
	rollback.StoreSnapshot(11, snapshot, 0xAABBCCDD);

	std::vector<std::byte> restored;
	const bool corrected = rollback.HandleCorrection(11, 0xABCD1234, 11,
	    [&](std::span<const std::byte> bytes) {
		    restored.assign(bytes.begin(), bytes.end());
		    return true;
	    },
	    [](std::span<const std::byte>) {});

	EXPECT_TRUE(corrected);
	EXPECT_EQ(restored, snapshot);
}

TEST(RollbackStateTest, RollbackReplayCorrectness)
{
	RollbackState rollback(16);
	rollback.StoreSnapshot(20, ToBytes({ 9 }), 0x10);
	rollback.QueuePredictedInput(21, ToBytes({ 1 }));
	rollback.QueuePredictedInput(22, ToBytes({ 2 }));
	rollback.QueuePredictedInput(23, ToBytes({ 3 }));

	std::vector<uint8_t> replayed;
	const bool corrected = rollback.HandleCorrection(20, 0x99, 23,
	    [](std::span<const std::byte>) {
		    return true;
	    },
	    [&](std::span<const std::byte> input) {
		    if (!input.empty())
			    replayed.push_back(static_cast<uint8_t>(input.front()));
	    });

	EXPECT_TRUE(corrected);
	EXPECT_EQ(replayed, (std::vector<uint8_t> { 1, 2, 3 }));
}

TEST(RollbackStateTest, DivergenceDetectionPath)
{
	RollbackState rollback(4);
	rollback.StoreSnapshot(7, ToBytes({ 0xAA }), 0x1111);

	EXPECT_FALSE(rollback.DetectDivergence(7, 0x1111));
	EXPECT_TRUE(rollback.DetectDivergence(7, 0x2222));
	EXPECT_FALSE(rollback.DetectDivergence(999, 0x2222));
}

} // namespace devilution::dvlnet
