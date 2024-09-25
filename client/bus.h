#pragma once

#include <asio/awaitable.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace tt09_levenshtein
{

class Bus
{
public:
    virtual ~Bus() = default;

    virtual asio::awaitable<void> read(std::uint32_t address, std::span<std::byte> buffer) = 0;
    virtual asio::awaitable<void> write(std::uint32_t address, std::span<const std::byte> data) = 0;
};

} // namespace tt09_levenshtein