#include "nthread.h"

#include <gtest/gtest.h>

namespace devilution {

TEST(SimTickHashTest, RecordsTickCountAndDeterministicHash)
{
	SimTickCount = 0;
	SimStateHash = 0;

	nthread_RecordSimStateHash(1234);
	EXPECT_EQ(SimTickCount, 1u);
	const uint32_t firstHash = SimStateHash;
	EXPECT_NE(firstHash, 0u);

	nthread_RecordSimStateHash(1234);
	EXPECT_EQ(SimTickCount, 2u);
	EXPECT_NE(SimStateHash, firstHash);

	SimTickCount = 0;
	SimStateHash = 0;
	nthread_RecordSimStateHash(1234);
	EXPECT_EQ(SimStateHash, firstHash);
}

} // namespace devilution
