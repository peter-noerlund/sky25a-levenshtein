#include "client.h"
#include "context.h"
#include "uart.h"
#include "uart_bus.h"
#include "real_context.h"
#include "real_uart.h"
#include "verilator_context.h"
#include "verilator_uart.h"

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/this_coro.hpp>
#include <lyra/lyra.hpp>
#include <fmt/printf.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>

namespace tt09_levenshtein
{

asio::awaitable<void> run(Context& context, Client& client)
{
    try
    {
        co_await context.init();
        co_await client.init();
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
    }

    co_return;
}

} // namespace tt09_levenshtein

int main(int argc, char** argv)
{
    bool showHelp = false;
    std::optional<std::filesystem::path> devicePath;
    std::optional<std::filesystem::path> vcdPath;
    std::optional<std::filesystem::path> dictionaryPath;
    std::string searchWord;

    auto cli = lyra::cli()
        | lyra::opt(devicePath, "DEVICE")["-d"]["--device"]("Use device instead of verilog")
        | lyra::opt(vcdPath, "FILE")["-v"]["--vcd-file"]("Create VCD file")
        | lyra::opt(dictionaryPath, "FILE")["-l"]["--load-dictionary"]("Load dictionary")
        | lyra::opt(searchWord, "WORD")["-s"]["--search"]("Search for word")
        | lyra::help(showHelp);

    auto result = cli.parse({argc, argv});
    if (!result)
    {
        fmt::println(stderr, "Error parsing command line arguments: {}", result.message());
        return EXIT_FAILURE;
    }
    if (showHelp)
    {
        std::cout << cli << std::endl;
        return EXIT_SUCCESS;
    }

    asio::io_context ioContext;

    try
    {
        using namespace tt09_levenshtein;

        std::unique_ptr<Context> context;
        std::unique_ptr<Uart> uart;
        if (devicePath)
        {
            context = std::make_unique<RealContext>();
            uart = std::make_unique<RealUart>(ioContext.get_executor(), *devicePath);
        }
        else
        {
            std::unique_ptr<VerilatorContext> verilatorContext;
            if (vcdPath)
            {
                verilatorContext = std::make_unique<VerilatorContext>(48000000, *vcdPath);
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

        asio::co_spawn(ioContext, run(*context, client), asio::detached);

        ioContext.run();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}