#include "runner.h"

#include "client.h"
#include "context.h"
#include "icestick_spi.h"
#include "levenshtein.h"
#include "real_context.h"
#include "spi.h"
#include "spi_bus.h"
#include "test_set.h"
#include "verilator_context.h"
#include "verilator_spi.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/this_coro.hpp>
#include <fmt/printf.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace tt09_levenshtein
{

Runner::Runner(Device device, Client::ChipSelect memoryChipSelect)
    : m_device(device)
    , m_memoryChipSelect(memoryChipSelect)
{
}

void Runner::setVcdPath(const std::filesystem::path& vcdPath)
{
    m_vcdPath = vcdPath;
}
    
void Runner::run(const Config& config)
{
    asio::io_context ioContext;
    
    std::unique_ptr<Context> context;
    std::unique_ptr<Spi> spi;

    switch (m_device)
    {
        case Device::Verilator:
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
            spi = std::make_unique<VerilatorSpi>(*verilatorContext);
            context = std::move(verilatorContext);
            break;
        }

        case Device::Icestick:
            context = std::make_unique<RealContext>();
            spi = std::make_unique<IcestickSpi>(*context);
            break;
    }

    SpiBus bus(*spi);

    Client client(*context, bus);

    asio::co_spawn(ioContext, run(ioContext, *context, client, config), asio::detached);

    ioContext.run();
}

asio::awaitable<void> Runner::run(asio::io_context& ioContext, Context& context, Client& client, const Config& config)
{
    try
    {
        co_await context.init();

        fmt::println("Initializing");
        co_await client.init(m_memoryChipSelect);

        if (config.dictionaryPath)
        {
            fmt::println("Reading dictionary {}", config.dictionaryPath->string());
            readDictionary(*config.dictionaryPath);

            if (!config.noLoad)
            {
                fmt::println("Loading dictionary");
                co_await loadDictionary(client);
            }
        }

        if (!config.searchWord.empty())
        {
            fmt::println("Searching for \"{}\"", config.searchWord);
            auto result = co_await client.search(config.searchWord);
            fmt::println("Best match is index {} ({}) with a distance of {}", result.index, result.index < m_dictionary.size() ? std::string_view(m_dictionary.at(result.index)) : std::string_view(), result.distance);
        }

        if (config.runTest)
        {
            co_await runTest(client, config);
        }
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
    }
    ioContext.stop();
}

asio::awaitable<void> Runner::runTest(Client& client, const Config& config)
{
    TestSet::Config testConfig;
    testConfig.minChar = 'a';
    testConfig.maxChar = 'a' + config.testAlphabetSize - 1;
    testConfig.minDictionaryWordLength = 1;
    testConfig.maxDictionaryWordLength = std::min(255U, client.maxLength() * 2);
    testConfig.dictionaryWordCount = config.testDictionarySize;
    testConfig.minSearchWordLength = 1;
    testConfig.maxSearchWordLength = client.maxLength();
    testConfig.searchWordCount = config.testSearchCount;

    TestSet testSet(testConfig);

    fmt::println("Loading dictionary of {} word(s)", config.testDictionarySize);
    auto dictionaryWords = testSet.dictionaryWords();
    co_await client.loadDictionary(dictionaryWords);

    fmt::println("Verifying dictionary");
    co_await client.verifyDictionary(dictionaryWords);

    fmt::println("Searching for {} word(s)", config.testSearchCount);

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

void Runner::readDictionary(const std::filesystem::path& path)
{
    std::ifstream stream(path.string().c_str());
    if (!stream.good())
    {
        throw std::runtime_error(fmt::format("Error opening dictionary: {}", path.string()));
    }

    m_dictionary.clear();

    std::string line;
    while (std::getline(stream, line).good())
    {
        std::string_view word = line;
        while (!word.empty() && std::isspace(word.back()))
        {
            word.remove_suffix(1);
        }

        m_dictionary.emplace_back(word);
    }
}

asio::awaitable<void> Runner::loadDictionary(Client& client)
{
    co_await client.loadDictionary(m_dictionary);
}

} // namespace tt09_levenshtein