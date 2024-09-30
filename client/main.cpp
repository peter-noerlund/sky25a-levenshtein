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
    bool noInit = false;
    bool runRandomizedTest = false;
    std::string deviceName = "verilog";
    std::string chipSelectName = "None";
    std::optional<std::filesystem::path> vcdPath;
    std::optional<std::filesystem::path> dictionaryPath;
    std::string searchWord;

    auto cli = lyra::cli()
        | lyra::opt(deviceName, "DEVICE")["-d"]["--device"]("Device type").choices("verilator", "icestick")
        | lyra::opt(chipSelectName, "PIN")["-c"]["--chip-select"]("Memory chip select pin").choices("none", "cs", "cs2", "cs3")
        | lyra::opt(vcdPath, "FILE")["-v"]["--vcd-file"]("Create VCD file")
        | lyra::opt(noInit)["--no-init"]("Skip initialization")
        | lyra::opt(dictionaryPath, "FILE")["-l"]["--load-dictionary"]("Load dictionary")
        | lyra::opt(searchWord, "WORD")["-s"]["--search"]("Search for word")
        | lyra::opt(runRandomizedTest)["-t"]["--test"]("Run randomized test")
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
    if (deviceName == "icestick")
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
        runner.run(dictionaryPath, searchWord, noInit, runRandomizedTest);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}