#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <filesystem>
#include <optional>
#include <string_view>
#include <string>

namespace tt09_levenshtein
{

class Context;
class Client;

class Runner
{
public:
    void setDevicePath(const std::filesystem::path& devicePath);
    void setVcdPath(const std::filesystem::path& vcdPath);
    
    void run(const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit);

private:
    asio::awaitable<void> run(asio::io_context& ioContext, Context& context, Client& client, const std::optional<std::filesystem::path>& dictionaryPath, std::string_view searchWord, bool noInit);
    asio::awaitable<void> loadDictionary(Client& client, const std::filesystem::path& path);

    std::optional<std::filesystem::path> m_devicePath;
    std::optional<std::filesystem::path> m_vcdPath;
};

} // namespace tt09_levenshtein