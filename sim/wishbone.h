#pragma once

#include "nonstd/lazy.h"
#include "simulator.h"

#include <fmt/format.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

template<typename Vtop, typename Transmitter>
class Wishbone
{
public:
    Wishbone(Simulator<Vtop>& simulator, Transmitter& transmitter) noexcept
        : m_sim(simulator)
        , m_transmitter(transmitter)
    {
    }

    nonstd::lazy<void> write(std::uint32_t address, std::uint8_t data)
    {
        auto res = co_await exec(Operation::Write, address, data);
        if (res != 0)
        {
            throw std::runtime_error(fmt::format("Wishbone error: 0x{:02x}", res).c_str());
        }
    }

    nonstd::lazy<void> write(std::uint32_t address, std::span<const std::uint8_t> data)
    {
        for (auto b : data)
        {
            co_await write(address++, b);
        }
    }

    nonstd::lazy<std::uint8_t> read(std::uint32_t address)
    {
        co_return co_await exec(Operation::Read, address);
    }

    nonstd::lazy<std::vector<std::uint8_t>> read(std::uint32_t address, std::size_t count)
    {
        std::vector<std::uint8_t> buffer;
        buffer.reserve(count);

        for (std::size_t i = 0; i != count; ++i)
        {
            buffer.push_back(co_await read(address++));
        }

        co_return buffer;
    }

private:
    enum class Operation
    {
        Read,
        Write
    };

    nonstd::lazy<std::uint8_t> exec(Operation operation, std::uint32_t address, std::uint8_t data = 0)
    {
        if (address & 0xFF800000)
        {
            throw std::invalid_argument(fmt::format("Inaccessible address: {:x}", address).c_str());
        }

        auto command = std::to_array<std::uint8_t>({
            static_cast<std::uint8_t>((operation == Operation::Write ? 0x80 : 0x00) | ((address >> 16) & 0x7F)),
            static_cast<std::uint8_t>(address >> 8),
            static_cast<std::uint8_t>(address),
            data
        });
        co_await m_transmitter.send(command);

        co_return co_await m_transmitter.recv();
    }

    Simulator<Vtop>& m_sim;
    Transmitter& m_transmitter;
};