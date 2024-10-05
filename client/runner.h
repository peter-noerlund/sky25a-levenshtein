#pragma once

#include "client.h"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <filesystem>
#include <map>
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
    struct Config
    {
        Device device = Device::Verilator;
        std::optional<std::filesystem::path> dictionaryPath;
        std::string searchWord;
        bool noClear = false;
        bool noLoadDictionary = false;
        bool runTest = false;
        bool verifyDictionary = false;
        bool verifySearch = false;
        unsigned int testAlphabetSize = 6;
        unsigned int testDictionarySize = 1024;
        unsigned int testSearchCount = 256;
    };

    Runner(Device device, Client::ChipSelect memoryChipSelect);
    void setVcdPath(const std::filesystem::path& vcdPath);
    
    void run(const Config& config);

private:
    asio::awaitable<void> run(asio::io_context& ioContext, Context& context, Client& client, const Config& config);
    asio::awaitable<void> init(Client& client, const Config& config);
    void readDictionary(const std::filesystem::path& path);
    void createCharset();
    void mapDictionaryToCharset();
    asio::awaitable<void> loadDictionary(Client& client);
    asio::awaitable<void> verifyDictionary(Client& client);
    asio::awaitable<void> search(Client& client, const Config& config, std::string_view word);
    asio::awaitable<void> runTest(Client& client, const Config& config);
    std::string mapStringToCharset(std::string_view string) const;

    Device m_device;
    Client::ChipSelect m_memoryChipSelect;
    std::optional<std::filesystem::path> m_vcdPath;
    std::vector<std::string> m_dictionary;
    std::vector<std::string> m_mappedDictionary;
    std::map<char32_t, char> m_charset;
};

} // namespace tt09_levenshtein