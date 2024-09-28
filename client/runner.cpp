#include "runner.h"

#include "client.h"
#include "context.h"
#include "levenshtein.h"
#include "uart.h"
#include "uart_bus.h"
#include "real_context.h"
#include "real_uart.h"
#include "test_set.h"
#include "verilator_context.h"
#include "verilator_spi_bus.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/this_coro.hpp>
#include <fmt/printf.h>

#include <cctype>
#include <cstdio>
#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace tt09_levenshtein
{

void Runner::setDevicePath(const std::filesystem::path& devicePath)
{
    m_devicePath = devicePath;
}

void Runner::setVcdPath(const std::filesystem::path& vcdPath)
{
    m_vcdPath = vcdPath;
}
    
void Runner::run(const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit, bool runRandomizedTest)
{
    asio::io_context ioContext;
    
    std::unique_ptr<Context> context;
    std::unique_ptr<Uart> uart;
    std::unique_ptr<Bus> bus;
    if (m_devicePath)
    {
        context = std::make_unique<RealContext>();
        uart = std::make_unique<RealUart>(ioContext.get_executor(), *m_devicePath);
        bus = std::make_unique<UartBus>(*uart);
    }
    else
    {
        std::unique_ptr<VerilatorContext> verilatorContext;
        if (m_vcdPath)
        {
            verilatorContext = std::make_unique<VerilatorContext>(50000000, *m_vcdPath);
        }
        else
        {
            verilatorContext = std::make_unique<VerilatorContext>(50000000);
        }
        bus = std::make_unique<VerilatorSpiBus>(*verilatorContext, 4);
        context = std::move(verilatorContext);
    }

    Client client(*context, *bus);

    asio::co_spawn(ioContext, run(ioContext, *context, client, dictionaryPath, searchWord, noInit, runRandomizedTest), asio::detached);

    ioContext.run();
}

asio::awaitable<void> Runner::run(asio::io_context& ioContext, Context& context, Client& client, const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit, bool runRandomizedTest)
{
    try
    {
        co_await context.init();
        if (!noInit)
        {
            fmt::println("Initializing");
            co_await client.init();
        }

        if (dictionaryPath)
        {
            fmt::println("Loading dictionary");
            co_await loadDictionary(client, *dictionaryPath);
        }

        if (!searchWord.empty())
        {
            fmt::println("Searching for \"{}\"", searchWord);

            auto result = co_await client.search(searchWord);

            fmt::println("Search result for \"{}\": index={} distance={}", searchWord, result.index, result.distance);
        }

        if (runRandomizedTest)
        {
            fmt::println("Running randomized test");
            co_await runTest(client);
        }
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
    }
    ioContext.stop();
}

asio::awaitable<void> Runner::runTest(Client& client)
{
    TestSet::Config testConfig;
    testConfig.minChar = 'a';
    testConfig.maxChar = 'f';
    testConfig.minDictionaryWordLength = 1;
    testConfig.maxDictionaryWordLength = 32;
    testConfig.dictionaryWordCount = 1024;
    testConfig.minSearchWordLength = 1;
    testConfig.maxSearchWordLength = 16;
    testConfig.searchWordCount = 256;

    TestSet testSet(testConfig);

    fmt::println("Loading random dictionary");
    auto dictionaryWords = testSet.dictionaryWords();
    co_await client.loadDictionary(dictionaryWords);

    fmt::println("Searching for words");

    for (const auto& searchWord : testSet.searchWords())
    {
        auto result = co_await client.search(searchWord);
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

asio::awaitable<void> Runner::loadDictionary(Client& client, const std::filesystem::path& path)
{
    std::ifstream stream(path.string().c_str());
    if (!stream.good())
    {
        throw std::runtime_error("Error opening dictionary");
    }

    std::vector<std::string> words;

    std::string line;
    while (std::getline(stream, line).good())
    {
        std::string_view word = line;
        while (!word.empty() && std::isspace(word.back()))
        {
            word.remove_suffix(1);
        }

        words.emplace_back(word);
    }

    co_await client.loadDictionary(words);
}

} // namespace tt09_levenshtein