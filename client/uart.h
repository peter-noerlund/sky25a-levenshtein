#pragma once

#include <asio/awaitable.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace tt09_levenshtein
{

class Uart
{
public:
    virtual ~Uart() = default;

    virtual asio::awaitable<void> send(std::span<const std::byte> data) = 0;
    virtual asio::awaitable<void> recv(std::span<std::byte> buffer) = 0;
};

} // namespace tt09_levenshtein