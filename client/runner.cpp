#include "runner.h"

#include "client.h"
#include "context.h"
#include "icestick_spi.h"
#include "levenshtein.h"
#include "real_context.h"
#include "spi.h"
#include "spi_bus.h"
#include "test_set.h"
#include "unicode.h"
#include "verilator_context.h"
#include "verilator_spi.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/this_coro.hpp>
#include <fmt/printf.h>

#include <algorithm>
#include <cctype>
#include <chrono>
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

        co_await init(client, config);

        if (config.dictionaryPath)
        {
            readDictionary(*config.dictionaryPath);
            createCharset();
            mapDictionaryToCharset();

            if (!config.noLoadDictionary)
            {
                co_await loadDictionary(client);
            }

            if (config.verifyDictionary)
            {
                co_await verifyDictionary(client);
            }
        }

        if (!config.searchWord.empty())
        {
            co_await search(client, config, config.searchWord);
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

asio::awaitable<void> Runner::init(Client& client, const Config& config)
{
    fmt::println("Initializing device");
    auto t1 = std::chrono::high_resolution_clock::now();
    co_await client.init(m_memoryChipSelect, !config.noClear);
    auto t2 = std::chrono::high_resolution_clock::now();
    fmt::println("Initialized device in \033[36m{}\033[0m ms", std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
}

asio::awaitable<void> Runner::search(Client& client, const Config& config, std::string_view word)
{
    auto mappedWord = mapStringToCharset(word);

    auto t1 = std::chrono::high_resolution_clock::now();
    auto result = co_await client.search(mappedWord);
    auto t2 = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    fmt::print("Best match for \033[33m{}\033[0m is ", word);
    if (result.index < m_dictionary.size())
    {
        fmt::print("\033[33m{}\033[0m", m_dictionary.at(result.index));
    }
    else
    {
        fmt::print("index \033[33m{}\033[0m", result.index);
    }
    fmt::print(" with a distance of \033[35m{}\033[0m.", result.distance);
    
    if (result.index < m_dictionary.size() && config.verifySearch)
    {
        auto distance = levenshtein(Unicode::toUTF32(word), Unicode::toUTF32(m_dictionary.at(result.index)));
        if (result.distance == distance)
        {
            fmt::print(" [\033[32mCORRECT\033[0m]");
        }
        else
        {
            fmt::print(" [\033[31mINCORRECT\033[0m should have been \033[35m{}\033[0m]", distance);
        }
    }
    fmt::println(" Search took \033[36m{}\033[0m ms", elapsed.count());
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

    auto testDictionary = testSet.dictionaryWords();
    m_dictionary.assign(testDictionary.begin(), testDictionary.end());
    
    createCharset();
    mapDictionaryToCharset();
    co_await loadDictionary(client);
    if (config.verifyDictionary)
    {   
        co_await verifyDictionary(client);
    }

    for (const auto& searchWord : testSet.searchWords())
    {
        co_await search(client, config, searchWord);
    }
}

void Runner::readDictionary(const std::filesystem::path& path)
{
    fmt::println("Reading dictionary: {}", path.string());

    auto t1 = std::chrono::high_resolution_clock::now();

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

    auto t2 = std::chrono::high_resolution_clock::now();

    fmt::println("Read dictionary in \033[36m{}\033[0m ms", std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
}

void Runner::createCharset()
{
    fmt::println("Creating character set");
    auto t1 = std::chrono::high_resolution_clock::now();
    m_charset.clear();
    for (const auto& word : m_dictionary)
    {
        for (auto c : Unicode::toUTF32(word))
        {
            if (!m_charset.contains(c))
            {
                // Note that 00 = end of word, 01 = end of dictionary and 02 = unknown character
                if (m_charset.size() == 256 - 3)
                {
                    throw std::out_of_range("Too many distinct characters in dictionary");
                }
                m_charset[c] = static_cast<char>(m_charset.size() + 3);
            }
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    fmt::println("Created character set of {} characters in \033[36m{}\033[0m ms", m_charset.size(), std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
}

void Runner::mapDictionaryToCharset()
{
    m_mappedDictionary.clear();

    fmt::println("Mapping dictionary to character set");

    auto t1 = std::chrono::high_resolution_clock::now();
    m_mappedDictionary.clear();
    m_mappedDictionary.reserve(m_dictionary.size());
    for (const auto& string : m_dictionary)
    {
        m_mappedDictionary.push_back(mapStringToCharset(string));
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    fmt::println("Mapped dictionary in \033[36m{}\033[0m ms", std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
}

asio::awaitable<void> Runner::loadDictionary(Client& client)
{
    fmt::println("Loading dictionary onto device");
    auto t1 = std::chrono::high_resolution_clock::now();
    co_await client.loadDictionary(m_mappedDictionary);
    auto t2 = std::chrono::high_resolution_clock::now();
    fmt::println("Loaded dictionary in \033[36m{}\033[0m ms", std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
}

asio::awaitable<void> Runner::verifyDictionary(Client& client)
{
    fmt::println("Verifying dictionary");
    auto t1 = std::chrono::high_resolution_clock::now();
    co_await client.verifyDictionary(m_mappedDictionary);
    auto t2 = std::chrono::high_resolution_clock::now();
    fmt::println("Verified dictionary in \033[36m{}\033[0m ms", std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
}

std::string Runner::mapStringToCharset(std::string_view string) const
{
    if (m_charset.empty())
    {
        return std::string(string);
    }

    auto u32string = Unicode::toUTF32(string);

    std::string buffer;
    buffer.reserve(u32string.size());
    for (auto c : u32string)
    {
        auto it = m_charset.find(c);
        buffer.push_back(it == m_charset.end() ? char(2) : it->second);
    }

    return buffer;
}

} // namespace tt09_levenshtein