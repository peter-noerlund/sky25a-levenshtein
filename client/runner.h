#pragma once

#include "client.h"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <filesystem>
#include <optional>
#include <string_view>
#include <string>

namespace tt09_levenshtein
{

class Context;

class Runner
{
public:
    enum class Device
    {
        Verilator,
        Icestick
    };

    Runner(Device device, Client::ChipSelect memoryChipSelect);
    void setVcdPath(const std::filesystem::path& vcdPath);
    
    void run(const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit, bool runRandomizedTest);

private:
    asio::awaitable<void> run(asio::io_context& ioContext, Context& context, Client& client, const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit, bool runRandomizedTest);
    asio::awaitable<void> loadDictionary(Client& client, const std::filesystem::path& path);
    asio::awaitable<void> runTest(Client& client);

    Device m_device;
    Client::ChipSelect m_memoryChipSelect;
    std::optional<std::filesystem::path> m_vcdPath;
};

} // namespace tt09_levenshtein