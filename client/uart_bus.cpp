#include "uart_bus.h"

#include <array>
#include <stdexcept>

namespace tt09_levenshtein
{

UartBus::UartBus(Uart& uart) noexcept
    : m_uart(uart)
{
}

asio::awaitable<std::byte> UartBus::execute(std::uint32_t command)
{
    auto data = std::to_array<std::uint8_t>({
        static_cast<std::uint8_t>(command >> 24),
        static_cast<std::uint8_t>(command >> 16),
        static_cast<std::uint8_t>(command >> 8),
        static_cast<std::uint8_t>(command)
    });

    co_await m_uart.send(std::as_bytes(std::span(data)));

    std::byte response;
    co_await m_uart.recv({&response, 1});

    co_return response;
}


} // namespace tt09_levenshtein