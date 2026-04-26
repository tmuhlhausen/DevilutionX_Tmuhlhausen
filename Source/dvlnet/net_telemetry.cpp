#include "dvlnet/net_telemetry.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>

#include <fmt/format.h>

namespace devilution {
namespace {

constexpr float Smoothing = 0.25F;

void Smooth(float &current, float sample)
{
	current += (sample - current) * Smoothing;
}

std::ofstream &TraceStream()
{
	static std::ofstream stream;
	return stream;
}

std::mutex &TelemetryMutex()
{
	static std::mutex mutex;
	return mutex;
}

bool ParseFloat(std::string_view input, float &value)
{
	const std::string storage(input);
	const char *begin = storage.c_str();
	char *end = nullptr;
	value = std::strtof(begin, &end);
	return end == begin + static_cast<std::ptrdiff_t>(storage.size());
}

bool ParseUInt64(std::string_view input, uint64_t &value)
{
	auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);
	return ec == std::errc {} && ptr == input.data() + input.size();
}

bool ParseUInt32(std::string_view input, uint32_t &value)
{
	auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);
	return ec == std::errc {} && ptr == input.data() + input.size();
}

} // namespace

NetTelemetryAggregator &GetNetTelemetryAggregator()
{
	static NetTelemetryAggregator telemetry;
	return telemetry;
}

void NetTelemetryAggregator::SetTraceEnabled(bool enabled, NetTraceFormat format)
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	traceEnabled_ = enabled;
	traceFormat_ = format;
	if (!enabled) {
		if (TraceStream().is_open())
			TraceStream().close();
		return;
	}
	if (TraceStream().is_open())
		return;
	TraceStream().open(format == NetTraceFormat::Jsonl ? "net_tick_trace.jsonl" : "net_tick_trace.tsv", std::ios::out | std::ios::trunc);
	if (format == NetTraceFormat::Tsv && TraceStream().is_open()) {
		TraceStream() << "tick\trtt_ms\tjitter_ms\tdrop_pct\tresend_pct\tdivergence\trollback_ms\trtt_p50_ms\trtt_p95_ms\tjitter_p95_ms\tanom_latency\tanom_drop\tanom_div\n";
	}
}

void NetTelemetryAggregator::FlushTick()
{
	NetTickTelemetrySample sample;
	sample.tick = tick_;
	sample.rttMs = lastRttMs_;
	sample.jitterMs = rolling_.jitterMs;
	sample.divergenceCount = divergenceThisTick_;
	sample.rollbackMs = rollbackMsThisTick_;
	if (sendsThisTick_ > 0) {
		sample.dropPct = static_cast<float>(dropsThisTick_) * 100.0F / static_cast<float>(sendsThisTick_);
		sample.resendPct = static_cast<float>(resendsThisTick_) * 100.0F / static_cast<float>(sendsThisTick_);
	}
	PushWindowSample(rttWindow_, sample.rttMs);
	PushWindowSample(jitterWindow_, sample.jitterMs);
	sample.rttP50Ms = Percentile(rttWindow_, 0.50F);
	sample.rttP95Ms = Percentile(rttWindow_, 0.95F);
	sample.jitterP95Ms = Percentile(jitterWindow_, 0.95F);
	sample.anomalyLatency = hasRtt_ && sample.rttMs > (sample.rttP95Ms + 25.0F);
	sample.anomalyDropBurst = sample.dropPct > std::max(5.0F, rolling_.dropPct + 3.0F);
	sample.anomalyDivergence = sample.divergenceCount > std::max(2U, rolling_.divergenceCount + 1U);

	if (tick_ == 0) {
		rolling_ = sample;
	} else {
		Smooth(rolling_.rttMs, sample.rttMs);
		Smooth(rolling_.jitterMs, sample.jitterMs);
		Smooth(rolling_.dropPct, sample.dropPct);
		Smooth(rolling_.resendPct, sample.resendPct);
		rolling_.divergenceCount = sample.divergenceCount;
		Smooth(rolling_.rollbackMs, sample.rollbackMs);
		Smooth(rolling_.rttP50Ms, sample.rttP50Ms);
		Smooth(rolling_.rttP95Ms, sample.rttP95Ms);
		Smooth(rolling_.jitterP95Ms, sample.jitterP95Ms);
		rolling_.anomalyLatency = sample.anomalyLatency;
		rolling_.anomalyDropBurst = sample.anomalyDropBurst;
		rolling_.anomalyDivergence = sample.anomalyDivergence;
		rolling_.tick = sample.tick;
	}

	if (traceEnabled_ && TraceStream().is_open()) {
		TraceStream() << FormatLine(sample, traceFormat_) << '\n';
	}

	sendsThisTick_ = 0;
	resendsThisTick_ = 0;
	dropsThisTick_ = 0;
	divergenceThisTick_ = 0;
	rollbackMsThisTick_ = 0.0F;
}

void NetTelemetryAggregator::PushWindowSample(std::vector<float> &window, float value)
{
	if (window.size() >= PercentileWindowSize)
		window.erase(window.begin());
	window.push_back(value);
}

float NetTelemetryAggregator::Percentile(const std::vector<float> &window, float fraction) const
{
	if (window.empty())
		return 0.0F;
	std::vector<float> sorted = window;
	std::sort(sorted.begin(), sorted.end());
	const size_t index = static_cast<size_t>(std::clamp(fraction, 0.0F, 1.0F) * static_cast<float>(sorted.size() - 1));
	return sorted[index];
}

void NetTelemetryAggregator::NextTick()
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	FlushTick();
	++tick_;
}

void NetTelemetryAggregator::RecordRtt(float rttMs)
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	if (hasRtt_)
		rolling_.jitterMs = std::abs(rttMs - lastRttMs_);
	lastRttMs_ = rttMs;
	hasRtt_ = true;
}

void NetTelemetryAggregator::RecordDrop()
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	++dropsThisTick_;
}

void NetTelemetryAggregator::RecordSend(bool resent)
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	++sendsThisTick_;
	if (resent)
		++resendsThisTick_;
}

void NetTelemetryAggregator::RecordDivergence()
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	++divergenceThisTick_;
}

void NetTelemetryAggregator::RecordRollbackMs(float rollbackMs)
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	rollbackMsThisTick_ = std::max(rollbackMsThisTick_, rollbackMs);
}

NetTickTelemetrySample NetTelemetryAggregator::RollingSample() const
{
	std::lock_guard<std::mutex> lock(TelemetryMutex());
	return rolling_;
}

std::string NetTelemetryAggregator::FormatLine(const NetTickTelemetrySample &sample, NetTraceFormat format)
{
	if (format == NetTraceFormat::Tsv) {
		return fmt::format("{}\t{:.2f}\t{:.2f}\t{:.2f}\t{:.2f}\t{}\t{:.2f}\t{:.2f}\t{:.2f}\t{:.2f}\t{}\t{}\t{}",
		    sample.tick,
		    sample.rttMs,
		    sample.jitterMs,
		    sample.dropPct,
		    sample.resendPct,
		    sample.divergenceCount,
		    sample.rollbackMs,
		    sample.rttP50Ms,
		    sample.rttP95Ms,
		    sample.jitterP95Ms,
		    sample.anomalyLatency ? 1 : 0,
		    sample.anomalyDropBurst ? 1 : 0,
		    sample.anomalyDivergence ? 1 : 0);
	}
	return fmt::format("{{\"tick\":{},\"rtt_ms\":{:.2f},\"jitter_ms\":{:.2f},\"drop_pct\":{:.2f},\"resend_pct\":{:.2f},\"divergence\":{},\"rollback_ms\":{:.2f},\"rtt_p50_ms\":{:.2f},\"rtt_p95_ms\":{:.2f},\"jitter_p95_ms\":{:.2f},\"anom_latency\":{},\"anom_drop\":{},\"anom_div\":{}}}",
	    sample.tick,
	    sample.rttMs,
	    sample.jitterMs,
	    sample.dropPct,
	    sample.resendPct,
	    sample.divergenceCount,
	    sample.rollbackMs,
	    sample.rttP50Ms,
	    sample.rttP95Ms,
	    sample.jitterP95Ms,
	    sample.anomalyLatency ? 1 : 0,
	    sample.anomalyDropBurst ? 1 : 0,
	    sample.anomalyDivergence ? 1 : 0);
}

std::optional<NetTickTelemetrySample> NetTelemetryAggregator::ParseLine(std::string_view line, NetTraceFormat format)
{
	NetTickTelemetrySample sample;
	if (format == NetTraceFormat::Tsv) {
		std::string_view fields[13];
		for (int i = 0; i < 13; ++i) {
			const size_t split = line.find('\t');
			if (split == std::string_view::npos) {
				if (i != 12)
					return std::nullopt;
				fields[i] = line;
				break;
			}
			fields[i] = line.substr(0, split);
			line.remove_prefix(split + 1);
		}
		if (!ParseUInt64(fields[0], sample.tick) || !ParseFloat(fields[1], sample.rttMs) || !ParseFloat(fields[2], sample.jitterMs)
		    || !ParseFloat(fields[3], sample.dropPct) || !ParseFloat(fields[4], sample.resendPct) || !ParseUInt32(fields[5], sample.divergenceCount)
		    || !ParseFloat(fields[6], sample.rollbackMs) || !ParseFloat(fields[7], sample.rttP50Ms) || !ParseFloat(fields[8], sample.rttP95Ms)
		    || !ParseFloat(fields[9], sample.jitterP95Ms)) {
			return std::nullopt;
		}
		uint32_t anomLatency = 0;
		uint32_t anomDrop = 0;
		uint32_t anomDiv = 0;
		if (!ParseUInt32(fields[10], anomLatency) || !ParseUInt32(fields[11], anomDrop) || !ParseUInt32(fields[12], anomDiv))
			return std::nullopt;
		sample.anomalyLatency = anomLatency != 0;
		sample.anomalyDropBurst = anomDrop != 0;
		sample.anomalyDivergence = anomDiv != 0;
		return sample;
	}

	unsigned long long tick = 0;
	unsigned int anomLatency = 0;
	unsigned int anomDrop = 0;
	unsigned int anomDiv = 0;
	int matched = std::sscanf(std::string(line).c_str(),
	    "{\"tick\":%llu,\"rtt_ms\":%f,\"jitter_ms\":%f,\"drop_pct\":%f,\"resend_pct\":%f,\"divergence\":%u,\"rollback_ms\":%f,\"rtt_p50_ms\":%f,\"rtt_p95_ms\":%f,\"jitter_p95_ms\":%f,\"anom_latency\":%u,\"anom_drop\":%u,\"anom_div\":%u}",
	    &tick,
	    &sample.rttMs,
	    &sample.jitterMs,
	    &sample.dropPct,
	    &sample.resendPct,
	    &sample.divergenceCount,
	    &sample.rollbackMs,
	    &sample.rttP50Ms,
	    &sample.rttP95Ms,
	    &sample.jitterP95Ms,
	    &anomLatency,
	    &anomDrop,
	    &anomDiv);
	if (matched != 13)
		return std::nullopt;
	sample.tick = tick;
	sample.anomalyLatency = anomLatency != 0;
	sample.anomalyDropBurst = anomDrop != 0;
	sample.anomalyDivergence = anomDiv != 0;
	return sample;
}

} // namespace devilution
