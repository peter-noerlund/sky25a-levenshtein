#include "basic_bus.h"

#include <stdexcept>

namespace tt09_levenshtein
{

asio::awaitable<void> BasicBus::read(std::uint32_t address, std::span<std::byte> buffer)
{
    for (auto& b : buffer)
    {
        b = co_await read(address++);
    }
}

asio::awaitable<std::byte> BasicBus::read(std::uint32_t address)
{
    co_return co_await execute(Operation::Read, address);
}

asio::awaitable<void> BasicBus::write(std::uint32_t address, std::span<const std::byte> data)
{
    for (auto b : data)
    {
        co_await write(address++, b);
    }
}

asio::awaitable<void> BasicBus::write(std::uint32_t address, std::byte value)
{
    co_await execute(Operation::Write, address, value);
}

asio::awaitable<std::byte> BasicBus::execute(Operation operation, std::uint32_t address, std::byte value)
{
    if (address > 0x7FFFFF)
    {
        throw std::invalid_argument("Address out of range");
    }

    std::uint32_t command = (operation == Operation::Write ? 0x80000000 : 0) | (address << 8) | std::to_integer<std::uint8_t>(value);

    co_return co_await execute(command);
}

} // namespace tt09_levenshtein