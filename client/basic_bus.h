#pragma once

#include "bus.h"

namespace tt09_levenshtein
{

class BasicBus : public Bus
{
public:
    asio::awaitable<void> read(std::uint32_t address, std::span<std::byte> buffer) override;
    asio::awaitable<void> write(std::uint32_t address, std::span<const std::byte> data) override;

protected:
    virtual asio::awaitable<std::byte> execute(std::uint32_t command) = 0;

private:
    enum class Operation
    {
        Read,
        Write
    };
    asio::awaitable<std::byte> execute(Operation operation, std::uint32_t address, std::byte value = {});

    asio::awaitable<std::byte> read(std::uint32_t address);
    asio::awaitable<void> write(std::uint32_t address, std::byte value);
};

} // namespace tt09_levenshtein