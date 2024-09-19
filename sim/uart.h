#pragma once

#include "nonstd/lazy.h"
#include "simulator.h"

#include <cstdint>
#include <span>
#include <stdexcept>

template<typename Vtop>
class Uart
{
public:
    explicit Uart(Simulator<Vtop>& simulator, unsigned int clockDivider = 16) noexcept
        : m_sim(simulator)
        , m_clockDivider(clockDivider)
    {
        m_sim.top().uart_rxd = 1;
    }       

    nonstd::lazy<void> send(std::uint8_t symbol)
    {
        // Start bit
        m_sim.top().uart_rxd = 0;
        co_await m_sim.clocks(m_clockDivider);

        // Data bits
        for (unsigned int i = 0; i != 8; ++i)
        {
            m_sim.top().uart_rxd = ((symbol & (1 << i)) ? 1 : 0);
            co_await m_sim.clocks(m_clockDivider);
        }

        // Stop bit
        m_sim.top().uart_rxd = 1;
        co_await m_sim.clocks(m_clockDivider);
    }

    nonstd::lazy<void> send(std::span<const std::uint8_t> symbols)
    {
        for (auto symbol : symbols)
        {
            co_await send(symbol);
        }
    }

    nonstd::lazy<std::uint8_t> recv()
    {
        // Start bit
        co_await m_sim.fallingEdge(m_sim.top().uart_txd);
        co_await m_sim.clocks(m_clockDivider);

        co_await m_sim.clocks(m_clockDivider / 2);

        // Symbols
        std::uint8_t symbol = 0;
        for (unsigned int i = 0; i != 8; ++i)
        {
            symbol |= (m_sim.top().uart_txd ? 1 : 0) << i;
            co_await m_sim.clocks(m_clockDivider);
        }

        // Stop bit
        if (m_sim.top().uart_txd != 1)
        {
            throw std::runtime_error("UART error");
        }

        co_await m_sim.clocks(m_clockDivider / 2);

        co_return symbol;
    }

private:
    Simulator<Vtop>& m_sim;
    unsigned int m_clockDivider;
};