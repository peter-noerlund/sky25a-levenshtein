#include "accelerator.h"
#include "nonstd/ensure_started.h"
#include "nonstd/lazy.h"
#include "uart.h"
#include "test_set.h"
#include "wishbone.h"
#include "Vtop.h"

#include <fmt/printf.h>
#include <verilated.h>

#include <array>
#include <string_view>

template<typename Vtop>
nonstd::lazy<void> runTest(Simulator<Vtop>& sim)
{
    try
    {
        using UartType = Uart<Vtop>;
        using WishboneType = Wishbone<Vtop, UartType>;
        using AcceleratorType = Accelerator<Vtop, WishboneType>;

        TestSet::Config testConfig;
        testConfig.minChar = 'a';
        testConfig.maxChar = 'f';
        testConfig.minDictionaryWordLength = 1;
        testConfig.maxDictionaryWordLength = 32;
        testConfig.dictionaryWordCount = 1024;
        testConfig.minSearchWordLength = 1;
        testConfig.maxSearchWordLength = 16;
        testConfig.searchWordCount = 32;

        TestSet testSet(testConfig);

        UartType uart(sim);
        WishboneType bus(sim, uart);
        AcceleratorType accel(sim, bus);

        co_await accel.init();

        fmt::println("Loading dictionary...");

        co_await accel.loadDictionary(testSet.dictionaryWords());

        fmt::println("Searching for words");

        for (const auto& searchWord : testSet.searchWords())
        {
            auto result = co_await accel.search(searchWord);
            fmt::println("  searchWord={}  idx={} ({})  distance={}", searchWord, result.index, testSet.dictionaryWords()[result.index], result.distance);
        }
    }
    catch (const std::exception& exception)
    {
        fmt::println("Caught exception: {}", exception.what());
    }

    sim.stop();
}

template<typename Vtop>
nonstd::lazy<void> stopAtClock(Simulator<Vtop>& sim, unsigned int maxClocks)
{
    co_await sim.clocks(maxClocks);
    fmt::println("Timeout");
    sim.stop();
}

int main(int argc, char** argv)
{
	Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    Simulator<Vtop> simulator;

    auto mainTest = runTest(simulator) | nonstd::ensure_started();
    auto abortTask = stopAtClock(simulator, 1024 * 1024 * 1024) | nonstd::ensure_started();

    simulator.run();

    return EXIT_SUCCESS;
}