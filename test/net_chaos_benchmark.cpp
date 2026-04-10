#include "dvlnet/net_chaos.hpp"

#include <array>

#include <benchmark/benchmark.h>

namespace devilution {

static void BM_NetChaosInjectorProcess(benchmark::State &state)
{
	NetChaosInjector injector(12345, NetChaosProfile { .dropRate = 0.15F, .duplicateRate = 0.05F, .reorderWindow = 4 });
	constexpr std::array<uint8_t, 32> packet {
	    0, 1, 2, 3, 4, 5, 6, 7,
	    8, 9, 10, 11, 12, 13, 14, 15,
	    16, 17, 18, 19, 20, 21, 22, 23,
	    24, 25, 26, 27, 28, 29, 30, 31,
	};

	for (auto _ : state) {
		auto out = injector.Process(NetPacket { packet });
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK(BM_NetChaosInjectorProcess);

} // namespace devilution
