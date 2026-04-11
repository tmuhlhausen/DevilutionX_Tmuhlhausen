#include "dvlnet/net_telemetry.hpp"

#include <algorithm>
#include <charconv>
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
		TraceStream() << "tick\trtt_ms\tjitter_ms\tdrop_pct\tresend_pct\tdivergence\trollback_ms\n";
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

	if (tick_ == 0) {
		rolling_ = sample;
	} else {
		Smooth(rolling_.rttMs, sample.rttMs);
		Smooth(rolling_.jitterMs, sample.jitterMs);
		Smooth(rolling_.dropPct, sample.dropPct);
		Smooth(rolling_.resendPct, sample.resendPct);
		rolling_.divergenceCount = sample.divergenceCount;
		Smooth(rolling_.rollbackMs, sample.rollbackMs);
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
		return fmt::format("{}\t{:.2f}\t{:.2f}\t{:.2f}\t{:.2f}\t{}\t{:.2f}",
		    sample.tick,
		    sample.rttMs,
		    sample.jitterMs,
		    sample.dropPct,
		    sample.resendPct,
		    sample.divergenceCount,
		    sample.rollbackMs);
	}
	return fmt::format("{{\"tick\":{},\"rtt_ms\":{:.2f},\"jitter_ms\":{:.2f},\"drop_pct\":{:.2f},\"resend_pct\":{:.2f},\"divergence\":{},\"rollback_ms\":{:.2f}}}",
	    sample.tick,
	    sample.rttMs,
	    sample.jitterMs,
	    sample.dropPct,
	    sample.resendPct,
	    sample.divergenceCount,
	    sample.rollbackMs);
}

std::optional<NetTickTelemetrySample> NetTelemetryAggregator::ParseLine(std::string_view line, NetTraceFormat format)
{
	NetTickTelemetrySample sample;
	if (format == NetTraceFormat::Tsv) {
		std::string_view fields[7];
		for (int i = 0; i < 7; ++i) {
			const size_t split = line.find('\t');
			if (split == std::string_view::npos) {
				if (i != 6)
					return std::nullopt;
				fields[i] = line;
				break;
			}
			fields[i] = line.substr(0, split);
			line.remove_prefix(split + 1);
		}
		if (!ParseUInt64(fields[0], sample.tick) || !ParseFloat(fields[1], sample.rttMs) || !ParseFloat(fields[2], sample.jitterMs)
		    || !ParseFloat(fields[3], sample.dropPct) || !ParseFloat(fields[4], sample.resendPct) || !ParseUInt32(fields[5], sample.divergenceCount)
		    || !ParseFloat(fields[6], sample.rollbackMs)) {
			return std::nullopt;
		}
		return sample;
	}

	unsigned long long tick = 0;
	int matched = std::sscanf(std::string(line).c_str(),
	    "{\"tick\":%llu,\"rtt_ms\":%f,\"jitter_ms\":%f,\"drop_pct\":%f,\"resend_pct\":%f,\"divergence\":%u,\"rollback_ms\":%f}",
	    &tick,
	    &sample.rttMs,
	    &sample.jitterMs,
	    &sample.dropPct,
	    &sample.resendPct,
	    &sample.divergenceCount,
	    &sample.rollbackMs);
	if (matched != 7)
		return std::nullopt;
	sample.tick = tick;
	return sample;
}

} // namespace devilution
