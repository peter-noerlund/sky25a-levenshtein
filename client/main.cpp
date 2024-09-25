#include "runner.h"

#include <lyra/lyra.hpp>
#include <fmt/printf.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>

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

    tt09_levenshtein::Runner runner;
    if (devicePath)
    {
        runner.setDevicePath(*devicePath);
    }
    if (vcdPath)
    {
        runner.setVcdPath(*vcdPath);
    }

    try
    {
        runner.run(dictionaryPath, searchWord);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}