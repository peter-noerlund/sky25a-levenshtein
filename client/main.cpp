#include "runner.h"

#include <lyra/lyra.hpp>
#include <fmt/printf.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>

int main(int argc, char** argv)
{
    bool showHelp = false;
    std::string interfaceName = "verilog";
    std::string chipSelectName = "cs";
    std::optional<std::filesystem::path> vcdPath;
    tt09_levenshtein::Runner::Config config;

    auto cli = lyra::cli()
        | lyra::opt(interfaceName, "DEVICE")["-i"]["--interface"]("Interface").choices("verilator", "icestick")
        | lyra::opt(chipSelectName, "PIN")["-c"]["--chip-select"]("Memory chip select pin").choices("cs", "cs2", "cs3")
        | lyra::opt(vcdPath, "FILE")["-v"]["--vcd-file"]("Create VCD file")
        | lyra::opt(config.dictionaryPath, "FILE")["-d"]["--dictionary"]("Dictionary")
        | lyra::opt(config.noClear)["--no-clear"]("Skip clearing vector map on initialization")
        | lyra::opt(config.noLoadDictionary)["--no-load-dictionary"]("Skip loading dictionary")
        | lyra::opt(config.verifyDictionary)["--verify-dictionary"]("Verify dictionary")
        | lyra::opt(config.searchWord, "WORD")["-s"]["--search"]("Search for word")
        | lyra::opt(config.verifySearch)["--verify-search"]("Verify search")
        | lyra::opt(config.runTest)["-t"]["--test"]("Run test")
        | lyra::opt(config.testAlphabetSize, "NUM")["--test-alphabet-size"]("Test alphabet size")
        | lyra::opt(config.testDictionarySize, "NUM")["--test-dictionary-size"]("Test dictionary size")
        | lyra::opt(config.testSearchCount, "NUM")["--test-search-count"]("Test search count")
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

    tt09_levenshtein::Runner::Device device;
    if (interfaceName == "icestick")
    {
        device = tt09_levenshtein::Runner::Device::Icestick;
    }
    else
    {
        device = tt09_levenshtein::Runner::Device::Verilator;
    }

    tt09_levenshtein::Client::ChipSelect chipSelect = tt09_levenshtein::Client::ChipSelect::None;
    if (chipSelectName == "cs")
    {
        chipSelect = tt09_levenshtein::Client::ChipSelect::CS;
    }
    else if (chipSelectName == "cs2")
    {
        chipSelect = tt09_levenshtein::Client::ChipSelect::CS2;
    }
    else if (chipSelectName == "cs3")
    {
        chipSelect = tt09_levenshtein::Client::ChipSelect::CS3;
    }

    tt09_levenshtein::Runner runner(device, chipSelect);
    if (vcdPath)
    {
        runner.setVcdPath(*vcdPath);
    }

    try
    {
        runner.run(config);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}