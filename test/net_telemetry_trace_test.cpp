#include "dvlnet/net_telemetry.hpp"

#include <gtest/gtest.h>

namespace devilution {

TEST(NetTelemetryTraceTest, ParsesJsonlRecordDeterministically)
{
	constexpr std::string_view Line = "{\"tick\":42,\"rtt_ms\":15.50,\"jitter_ms\":2.25,\"drop_pct\":3.00,\"resend_pct\":1.00,\"divergence\":7,\"rollback_ms\":4.75}";
	const std::optional<NetTickTelemetrySample> parsed = NetTelemetryAggregator::ParseLine(Line, NetTraceFormat::Jsonl);
	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(parsed->tick, 42U);
	EXPECT_FLOAT_EQ(parsed->rttMs, 15.5F);
	EXPECT_FLOAT_EQ(parsed->jitterMs, 2.25F);
	EXPECT_FLOAT_EQ(parsed->dropPct, 3.0F);
	EXPECT_FLOAT_EQ(parsed->resendPct, 1.0F);
	EXPECT_EQ(parsed->divergenceCount, 7U);
	EXPECT_FLOAT_EQ(parsed->rollbackMs, 4.75F);
}

TEST(NetTelemetryTraceTest, ParsesTsvRecordDeterministically)
{
	constexpr std::string_view Line = "42\t15.50\t2.25\t3.00\t1.00\t7\t4.75";
	const std::optional<NetTickTelemetrySample> parsed = NetTelemetryAggregator::ParseLine(Line, NetTraceFormat::Tsv);
	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(parsed->tick, 42U);
	EXPECT_FLOAT_EQ(parsed->rttMs, 15.5F);
	EXPECT_FLOAT_EQ(parsed->jitterMs, 2.25F);
	EXPECT_FLOAT_EQ(parsed->dropPct, 3.0F);
	EXPECT_FLOAT_EQ(parsed->resendPct, 1.0F);
	EXPECT_EQ(parsed->divergenceCount, 7U);
	EXPECT_FLOAT_EQ(parsed->rollbackMs, 4.75F);
}

TEST(NetTelemetryTraceTest, RejectsMalformedRecords)
{
	EXPECT_FALSE(NetTelemetryAggregator::ParseLine("nope", NetTraceFormat::Jsonl).has_value());
	EXPECT_FALSE(NetTelemetryAggregator::ParseLine("1\t2\t3", NetTraceFormat::Tsv).has_value());
}

} // namespace devilution
