#pragma once

#include <asio/awaitable.hpp>

#include <cstddef>
#include <span>

namespace tt09_levenshtein
{

class Spi
{
public:
    virtual ~Spi() = default;

    virtual asio::awaitable<void> enable() = 0;
    virtual asio::awaitable<void> xmit(std::span<const std::byte> data, std::span<std::byte> buffer) = 0;
    virtual asio::awaitable<void> disable() = 0;
};

} // namespace tt09_levenshtein