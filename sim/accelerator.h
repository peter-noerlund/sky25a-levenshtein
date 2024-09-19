#pragma once

#include "nonstd/lazy.h"
#include "simulator.h"

#include <fmt/format.h>

#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string_view>

template<typename Vtop, typename Wishbone>
class Accelerator
{
    enum ControlFlags : std::uint8_t
    {
        EnableFlag = 0x01,
        ErrorFlag = 0x02
    };

    enum ControlAddress : std::uint32_t
    {
        CtrlControlAddress = 0x0000000,
        CtrlLengthAddress = 0x000001,
        CtrlMaskAddress = 0x000002,
        CtrlVpAddress = 0x000004
    };

    enum StatusAddress : std::uint32_t
    {
        StatusStatusAddress = 0x0000000,
        StatusDistanceAddress = 0x0000001,
        StatusIndexAddress = 0x0000002,
    };

    enum BaseAddress : std::uint32_t
    {
        BaseBitvectorAddress = 0x400000,
        BaseDictionaryAddress = 0x600000
    };

public:
    struct Result
    {
        std::uint16_t index;
        std::uint8_t distance;
    };

    constexpr Accelerator(Simulator<Vtop>& simulator, Wishbone& wishbone) noexcept
        : m_sim(simulator)
        , m_bus(wishbone)
    {
        m_sim.top().rst_n = 0;
        m_sim.top().ena = 1;
    }

    nonstd::lazy<void> init()
    {
        m_sim.top().rst_n = 0;
        m_sim.top().ena = 1;
        co_await m_sim.clocks(10);
        m_sim.top().rst_n = 1;
        co_await m_sim.clocks(10);

        for (unsigned int i = 0; i != 256; ++i)
        {
            auto emptyBitVector = std::to_array<std::uint8_t>({0, 0});
            co_await m_bus.write(BaseBitvectorAddress + i * 2, emptyBitVector);
        }
    }

    template<typename Container>
    nonstd::lazy<void> loadDictionary(Container&& container)
    {
        std::uint32_t address = BaseDictionaryAddress;
        for (const auto& word : container)
        {
            if (word.size() > 255)
            {
                throw std::invalid_argument(fmt::format("Word \"{}\" exceeds 255 characters", word).c_str());
            }
            co_await m_bus.write(address, {reinterpret_cast<const std::uint8_t*>(word.data()), word.size()});
            address += word.size();
            co_await m_bus.write(address++, 0xFE);
        }
        co_await m_bus.write(address, 0xFF);
    }

    nonstd::lazy<Result> search(std::string_view word)
    {
        if (word.size() > 16)
        {
            throw std::invalid_argument(fmt::format("Word \"{}\" exceeds 16 characters", word).c_str());
        }

        // Verify accelerator is idle

        auto status = co_await m_bus.read(StatusStatusAddress);
        if ((status & EnableFlag) != 0)
        {
            throw std::runtime_error("Cannot search while another search is in progress");
        }

        // Generate bitvectors

        std::map<std::uint8_t, std::uint16_t> vectorMap;
        for (auto c : word)
        {
            std::uint16_t vector = 0;
            for (std::string_view::size_type i = 0; i != word.size(); ++i)
            {
                if (word[i] == c)
                {
                    vector |= (1 << i);
                }
            }
            vectorMap[static_cast<std::uint8_t>(c)] = vector;
        }

        // Write bitvectors
        std::array<std::uint8_t, 2> tmp;

        for (auto [c, vector] : vectorMap)
        {
            tmp = {static_cast<std::uint8_t>(vector >> 8), static_cast<std::uint8_t>(vector)};
            co_await m_bus.write(BaseBitvectorAddress + c * 2, tmp);
        }

        // Set up the rest

        co_await m_bus.write(CtrlLengthAddress, word.size());

        std::uint16_t mask = 1 << (word.size() - 1);
        tmp = {static_cast<std::uint8_t>(mask >> 8), static_cast<std::uint8_t>(mask)};
        co_await m_bus.write(CtrlMaskAddress, tmp);

        std::uint16_t vp = (1 << word.size()) - 1;
        tmp = {static_cast<std::uint8_t>(vp >> 8), static_cast<std::uint8_t>(vp)};
        co_await m_bus.write(CtrlVpAddress, tmp);

        // Initiate search

        co_await m_bus.write(CtrlControlAddress, EnableFlag);

        for (unsigned int i = 0; i != 20; ++i)
        {
            co_await m_sim.clocks(5000);

            status = co_await m_bus.read(StatusStatusAddress);
            if ((status & EnableFlag) == 0)
            {
                break;
            }
        }

        if ((status & EnableFlag) == EnableFlag)
        {
            throw std::runtime_error("Accelerator didn't finish in time");
        }
        if ((status & ErrorFlag) == ErrorFlag)
        {
            throw std::runtime_error("Accelerator failed with an error");
        }

        // Read result
        Result result;
        result.distance = co_await m_bus.read(StatusDistanceAddress);

        auto buffer = co_await m_bus.read(StatusIndexAddress, 2);
        result.index = (static_cast<std::uint16_t>(buffer[0]) << 8) | buffer[1];

        // Clear bitvectors
        for (auto [c, vector] : vectorMap)
        {
            tmp = {0, 0};
            co_await m_bus.write(BaseBitvectorAddress + c * 2, tmp);
        }
        co_return result;
    }

private:
    Simulator<Vtop>& m_sim;
    Wishbone& m_bus;
};