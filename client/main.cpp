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
    bool noInit = false;
    bool runRandomizedTest = false;
    std::string device = "verilog";
    std::optional<std::filesystem::path> vcdPath;
    std::optional<std::filesystem::path> dictionaryPath;
    std::string searchWord;

    auto cli = lyra::cli()
        | lyra::opt(device, "DEVICE")["-d"]["--device"]("Device type").choices("verilog", "icestick")
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

    tt09_levenshtein::Runner runner(device == "icestick" ? tt09_levenshtein::Runner::Device::Icestick : tt09_levenshtein::Runner::Device::Verilator);
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