#include "verilator_spi.h"

#include "verilator_context.h"

#include <cstdint>
#include <stdexcept>

namespace tt09_levenshtein
{

VerilatorSpi::VerilatorSpi(VerilatorContext& context, unsigned int clockDivider) noexcept
    : m_context(context)
    , m_clockDivider(clockDivider)
{
    m_context.top().spi_ss_n = 1;
}

asio::awaitable<void> VerilatorSpi::enable()
{
    m_context.top().spi_ss_n = 0;
    co_return;
}

asio::awaitable<void> VerilatorSpi::disable()
{
    m_context.top().spi_ss_n = 1;
    co_await m_context.clocks(m_clockDivider);
}

asio::awaitable<void> VerilatorSpi::xmit(std::span<const std::byte> data, std::span<std::byte> buffer)
{
    if (m_context.top().spi_ss_n)
    {
        throw std::logic_error("Transfer not allowed when SPI is disabled");
    }

    auto lowClocks = m_clockDivider / 2;
    auto highClocks = m_clockDivider - lowClocks;

    auto& sck = m_context.top().spi_sck;
    auto& mosi = m_context.top().spi_mosi;
    const auto& miso = m_context.top().spi_miso;

    for (auto b : data)
    {
        auto valueOut = std::to_integer<std::uint8_t>(b);

        for (unsigned int i = 0; i != 8; ++i)
        {
            sck = 0;
            mosi = valueOut >> 7;
            valueOut <<= 1;
            co_await m_context.clocks(lowClocks);
            
            sck = 1;
            co_await m_context.clocks(highClocks);
        }
    }

    for (auto& b : buffer)
    {
        std::uint8_t valueIn = 0;
        for (unsigned int i = 0; i != 8; ++i)
        {
            sck = 0;
            co_await m_context.clocks(lowClocks);
            
            valueIn = (valueIn << 1) | (miso ? 1 : 0);
            sck = 1;
            co_await m_context.clocks(highClocks);
        }

        b = std::byte(valueIn);
    }
}

} // namespace tt09_levenshtein