#include "uart_bus.h"

#include <array>
#include <stdexcept>

namespace tt09_levenshtein
{

UartBus::UartBus(Uart& uart) noexcept
    : m_uart(uart)
{
}

asio::awaitable<void> UartBus::read(std::uint32_t address, std::span<std::byte> buffer)
{
    for (auto& b : buffer)
    {
        b = co_await read(address++);
    }
}

asio::awaitable<std::byte> UartBus::read(std::uint32_t address)
{
    co_return co_await execute(Operation::Read, address);
}

asio::awaitable<void> UartBus::write(std::uint32_t address, std::span<const std::byte> data)
{
    for (auto b : data)
    {
        co_await write(address++, b);
    }
}

asio::awaitable<void> UartBus::write(std::uint32_t address, std::byte value)
{
    co_await execute(Operation::Write, address, value);
}

asio::awaitable<std::byte> UartBus::execute(Operation operation, std::uint32_t address, std::byte value)
{
    if (address > 0x7FFFFF)
    {
        throw std::invalid_argument("Address out of range");
    }

    auto command = std::to_array<std::uint8_t>({
        static_cast<std::uint8_t>((address >> 16) | (operation == Operation::Write ? 0x80 : 0x00)),
        static_cast<std::uint8_t>(address >> 8),
        static_cast<std::uint8_t>(address),
        std::to_integer<std::uint8_t>(value)
    });

    co_await m_uart.send(std::as_bytes(std::span(command)));

    std::byte response;
    co_await m_uart.recv({&response, 1});

    co_return response;
}


} // namespace tt09_levenshtein