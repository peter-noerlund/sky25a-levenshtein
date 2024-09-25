#include "verilator_uart.h"

#include "verilator_context.h"

#include <fmt/printf.h>

#include <stdexcept>

namespace tt09_levenshtein
{

VerilatorUart::VerilatorUart(VerilatorContext& context, unsigned int clockDivider) noexcept
    : m_context(context)
    , m_clockDivider(clockDivider)
{
    m_context.top().uart_rxd = 1;
}

asio::awaitable<void> VerilatorUart::send(std::span<const std::byte> data)
{
    for (auto b : data)
    {
        co_await send(b);
    }
}

asio::awaitable<void> VerilatorUart::send(std::byte value)
{
    auto symbol = std::to_integer<std::uint8_t>(value);

    // Start bit
    m_context.top().uart_rxd = 0;
    co_await m_context.clocks(m_clockDivider);

    // Data bits
    for (unsigned int i = 0; i != 8; ++i)
    {
        m_context.top().uart_rxd = ((symbol & (1 << i)) ? 1 : 0);
        co_await m_context.clocks(m_clockDivider);
    }

    // Stop bit
    m_context.top().uart_rxd = 1;
    co_await m_context.clocks(m_clockDivider);
}

asio::awaitable<void> VerilatorUart::recv(std::span<std::byte> buffer)
{
    for (auto& b : buffer)
    {
        b = co_await recv();
    }
}

asio::awaitable<std::byte> VerilatorUart::recv()
{
    // Start bit
    co_await m_context.fallingEdge(m_context.top().uart_txd);
    co_await m_context.clocks(m_clockDivider);
    
    co_await m_context.clocks(m_clockDivider / 2);

    std::uint8_t symbol = 0;
    for (unsigned int i = 0; i != 8; ++i)
    {
        symbol |= (m_context.top().uart_txd ? 1 : 0) << i;
        co_await m_context.clocks(m_clockDivider);
    }

    // Stop bit
    if (m_context.top().uart_txd != 1)
    {
        throw std::runtime_error("UART error");
    }

    co_await m_context.clocks(m_clockDivider / 2);

    co_return std::byte(symbol);
}

} // namespace tt09_levenshtein