#include "runner.h"

#include "client.h"
#include "context.h"
#include "uart.h"
#include "uart_bus.h"
#include "real_context.h"
#include "real_uart.h"
#include "verilator_context.h"
#include "verilator_uart.h"

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
    
void Runner::run(const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit)
{
    asio::io_context ioContext;
    
    std::unique_ptr<Context> context;
    std::unique_ptr<Uart> uart;
    if (m_devicePath)
    {
        context = std::make_unique<RealContext>();
        uart = std::make_unique<RealUart>(ioContext.get_executor(), *m_devicePath);
    }
    else
    {
        std::unique_ptr<VerilatorContext> verilatorContext;
        if (m_vcdPath)
        {
            verilatorContext = std::make_unique<VerilatorContext>(48000000, *m_vcdPath);
        }
        else
        {
            verilatorContext = std::make_unique<VerilatorContext>(48000000);
        }
        uart = std::make_unique<VerilatorUart>(*verilatorContext, 16);
        context = std::move(verilatorContext);
    }

    UartBus bus(*uart);
    Client client(*context, bus);

    asio::co_spawn(ioContext, run(ioContext, *context, client, dictionaryPath, searchWord, noInit), asio::detached);

    ioContext.run();
}

asio::awaitable<void> Runner::run(asio::io_context& ioContext, Context& context, Client& client, const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit)
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

        ioContext.stop();
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
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