#include "dvlnet/net_telemetry.hpp"

#include <gtest/gtest.h>

namespace devilution {

TEST(NetTelemetryTraceTest, ParsesJsonlRecordDeterministically)
{
	constexpr std::string_view Line = "{\"tick\":42,\"rtt_ms\":15.50,\"jitter_ms\":2.25,\"drop_pct\":3.00,\"resend_pct\":1.00,\"divergence\":7,\"rollback_ms\":4.75,\"rtt_p50_ms\":14.00,\"rtt_p95_ms\":21.00,\"jitter_p95_ms\":6.00,\"anom_latency\":1,\"anom_drop\":0,\"anom_div\":1}";
	const std::optional<NetTickTelemetrySample> parsed = NetTelemetryAggregator::ParseLine(Line, NetTraceFormat::Jsonl);
	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(parsed->tick, 42U);
	EXPECT_FLOAT_EQ(parsed->rttMs, 15.5F);
	EXPECT_FLOAT_EQ(parsed->jitterMs, 2.25F);
	EXPECT_FLOAT_EQ(parsed->dropPct, 3.0F);
	EXPECT_FLOAT_EQ(parsed->resendPct, 1.0F);
	EXPECT_EQ(parsed->divergenceCount, 7U);
	EXPECT_FLOAT_EQ(parsed->rollbackMs, 4.75F);
	EXPECT_FLOAT_EQ(parsed->rttP50Ms, 14.0F);
	EXPECT_FLOAT_EQ(parsed->rttP95Ms, 21.0F);
	EXPECT_FLOAT_EQ(parsed->jitterP95Ms, 6.0F);
	EXPECT_TRUE(parsed->anomalyLatency);
	EXPECT_FALSE(parsed->anomalyDropBurst);
	EXPECT_TRUE(parsed->anomalyDivergence);
}

TEST(NetTelemetryTraceTest, ParsesTsvRecordDeterministically)
{
	constexpr std::string_view Line = "42\t15.50\t2.25\t3.00\t1.00\t7\t4.75\t14.00\t21.00\t6.00\t1\t0\t1";
	const std::optional<NetTickTelemetrySample> parsed = NetTelemetryAggregator::ParseLine(Line, NetTraceFormat::Tsv);
	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(parsed->tick, 42U);
	EXPECT_FLOAT_EQ(parsed->rttMs, 15.5F);
	EXPECT_FLOAT_EQ(parsed->jitterMs, 2.25F);
	EXPECT_FLOAT_EQ(parsed->dropPct, 3.0F);
	EXPECT_FLOAT_EQ(parsed->resendPct, 1.0F);
	EXPECT_EQ(parsed->divergenceCount, 7U);
	EXPECT_FLOAT_EQ(parsed->rollbackMs, 4.75F);
	EXPECT_FLOAT_EQ(parsed->rttP50Ms, 14.0F);
	EXPECT_FLOAT_EQ(parsed->rttP95Ms, 21.0F);
	EXPECT_FLOAT_EQ(parsed->jitterP95Ms, 6.0F);
	EXPECT_TRUE(parsed->anomalyLatency);
	EXPECT_FALSE(parsed->anomalyDropBurst);
	EXPECT_TRUE(parsed->anomalyDivergence);
}

TEST(NetTelemetryTraceTest, RejectsMalformedRecords)
{
	EXPECT_FALSE(NetTelemetryAggregator::ParseLine("nope", NetTraceFormat::Jsonl).has_value());
	EXPECT_FALSE(NetTelemetryAggregator::ParseLine("1\t2\t3", NetTraceFormat::Tsv).has_value());
	EXPECT_FALSE(NetTelemetryAggregator::ParseLine("{\"tick\":1}", NetTraceFormat::Jsonl).has_value());
	EXPECT_FALSE(NetTelemetryAggregator::ParseLine("1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14", NetTraceFormat::Tsv).has_value());
}

TEST(NetTelemetryTraceTest, FormatAndParseRoundTripDeterministically)
{
	const NetTickTelemetrySample input {
		.tick = 99,
		.rttMs = 44.75F,
		.jitterMs = 3.25F,
		.dropPct = 0.5F,
		.resendPct = 7.25F,
		.divergenceCount = 2,
		.rollbackMs = 6.5F,
		.rttP50Ms = 42.0F,
		.rttP95Ms = 54.0F,
		.jitterP95Ms = 8.5F,
		.anomalyLatency = true,
		.anomalyDropBurst = false,
		.anomalyDivergence = true,
	};

	const std::string jsonl = NetTelemetryAggregator::FormatLine(input, NetTraceFormat::Jsonl);
	const std::optional<NetTickTelemetrySample> jsonlParsed = NetTelemetryAggregator::ParseLine(jsonl, NetTraceFormat::Jsonl);
	ASSERT_TRUE(jsonlParsed.has_value());
	EXPECT_EQ(jsonlParsed->tick, input.tick);
	EXPECT_FLOAT_EQ(jsonlParsed->rttMs, input.rttMs);
	EXPECT_FLOAT_EQ(jsonlParsed->resendPct, input.resendPct);
	EXPECT_EQ(jsonlParsed->divergenceCount, input.divergenceCount);
	EXPECT_TRUE(jsonlParsed->anomalyLatency);
	EXPECT_FALSE(jsonlParsed->anomalyDropBurst);
	EXPECT_TRUE(jsonlParsed->anomalyDivergence);

	const std::string tsv = NetTelemetryAggregator::FormatLine(input, NetTraceFormat::Tsv);
	const std::optional<NetTickTelemetrySample> tsvParsed = NetTelemetryAggregator::ParseLine(tsv, NetTraceFormat::Tsv);
	ASSERT_TRUE(tsvParsed.has_value());
	EXPECT_EQ(tsvParsed->tick, input.tick);
	EXPECT_FLOAT_EQ(tsvParsed->rollbackMs, input.rollbackMs);
	EXPECT_FLOAT_EQ(tsvParsed->rttP95Ms, input.rttP95Ms);
	EXPECT_FLOAT_EQ(tsvParsed->jitterP95Ms, input.jitterP95Ms);
}

TEST(NetTelemetryTraceTest, PercentilesAndAnomalyMarkersTrackSpikes)
{
	NetTelemetryAggregator telemetry;
	for (uint64_t tick = 0; tick < 16; ++tick) {
		telemetry.RecordRtt(20.0F + static_cast<float>(tick % 3));
		telemetry.RecordSend(false);
		telemetry.NextTick();
	}
	telemetry.RecordRtt(160.0F);
	telemetry.RecordSend(false);
	telemetry.RecordDrop();
	telemetry.RecordDivergence();
	telemetry.RecordDivergence();
	telemetry.RecordDivergence();
	telemetry.NextTick();
	const NetTickTelemetrySample sample = telemetry.RollingSample();
	EXPECT_GT(sample.rttP95Ms, sample.rttP50Ms);
	EXPECT_TRUE(sample.anomalyLatency);
	EXPECT_TRUE(sample.anomalyDropBurst);
	EXPECT_TRUE(sample.anomalyDivergence);
}

} // namespace devilution
