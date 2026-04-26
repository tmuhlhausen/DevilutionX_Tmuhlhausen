#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace devilution {

enum class NetTraceFormat {
	Jsonl,
	Tsv,
};

struct NetTickTelemetrySample {
	uint64_t tick = 0;
	float rttMs = 0.0F;
	float jitterMs = 0.0F;
	float dropPct = 0.0F;
	float resendPct = 0.0F;
	uint32_t divergenceCount = 0;
	float rollbackMs = 0.0F;
	float rttP50Ms = 0.0F;
	float rttP95Ms = 0.0F;
	float jitterP95Ms = 0.0F;
	bool anomalyLatency = false;
	bool anomalyDropBurst = false;
	bool anomalyDivergence = false;
};

class NetTelemetryAggregator {
public:
	void SetTraceEnabled(bool enabled, NetTraceFormat format = NetTraceFormat::Jsonl);
	void NextTick();
	void RecordRtt(float rttMs);
	void RecordDrop();
	void RecordSend(bool resent);
	void RecordDivergence();
	void RecordRollbackMs(float rollbackMs);

	[[nodiscard]] NetTickTelemetrySample RollingSample() const;

	[[nodiscard]] static std::string FormatLine(const NetTickTelemetrySample &sample, NetTraceFormat format);
	[[nodiscard]] static std::optional<NetTickTelemetrySample> ParseLine(std::string_view line, NetTraceFormat format);

private:
	void FlushTick();

	NetTickTelemetrySample rolling_ {};
	uint64_t tick_ = 0;
	float lastRttMs_ = 0.0F;
	bool hasRtt_ = false;
	uint32_t sendsThisTick_ = 0;
	uint32_t resendsThisTick_ = 0;
	uint32_t dropsThisTick_ = 0;
	uint32_t divergenceThisTick_ = 0;
	float rollbackMsThisTick_ = 0.0F;
	bool traceEnabled_ = false;
	NetTraceFormat traceFormat_ = NetTraceFormat::Jsonl;
	static constexpr size_t PercentileWindowSize = 64;
	std::vector<float> rttWindow_;
	std::vector<float> jitterWindow_;
	void PushWindowSample(std::vector<float> &window, float value);
	[[nodiscard]] float Percentile(const std::vector<float> &window, float fraction) const;
};

NetTelemetryAggregator &GetNetTelemetryAggregator();

} // namespace devilution
