#include "accelerator.h"
#include "levenshtein.h"
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

        auto dictionaryWords = testSet.dictionaryWords();

        co_await accel.loadDictionary(dictionaryWords);

        fmt::println("Searching for words");

        for (const auto& searchWord : testSet.searchWords())
        {
            auto result = co_await accel.search(searchWord);
            if (result.index >= dictionaryWords.size())
            {
                throw std::runtime_error(fmt::format("Result index out of range ({} >= {}). Search word was {}", result.index, dictionaryWords.size(), searchWord).c_str());
            }

            auto word = dictionaryWords[result.index];
            auto distance = levenshtein(searchWord, word);
            if (result.distance != distance)
            {
                throw std::runtime_error(fmt::format("Invalid distance. Got {}, expected {}. Search word was {}, result index was {} ({})", result.distance, distance, searchWord, result.index, word).c_str());
            }

            fmt::println("  {}: {} (distance {})", searchWord, word, result.distance);
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