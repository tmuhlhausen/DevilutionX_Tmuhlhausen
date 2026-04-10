#include "dvlnet/net_qos.hpp"

#include <gtest/gtest.h>

namespace devilution {

TEST(NetQosTest, DropsBudgetUnderLossAndLatency)
{
	NetPacketBudgetController controller(1000.0F);
	controller.Update(NetLinkMetrics { .rttMs = 30.0F, .jitterMs = 2.0F, .lossPct = 0.0F, .bytesPerSecondCap = 50000.0F });
	const int healthyBudget = controller.BudgetBytes();

	for (int i = 0; i < 10; ++i) {
		controller.Update(NetLinkMetrics { .rttMs = 250.0F, .jitterMs = 50.0F, .lossPct = 40.0F, .bytesPerSecondCap = 12000.0F });
	}
	const int degradedBudget = controller.BudgetBytes();
	EXPECT_LT(degradedBudget, healthyBudget);
}

TEST(NetQosTest, RespectsLowerBound)
{
	NetPacketBudgetController controller(1000.0F);
	for (int i = 0; i < 20; ++i) {
		controller.Update(NetLinkMetrics { .rttMs = 1000.0F, .jitterMs = 300.0F, .lossPct = 99.0F, .bytesPerSecondCap = 1000.0F });
	}
	EXPECT_GE(controller.BudgetBytes(), 100);
}

} // namespace devilution
