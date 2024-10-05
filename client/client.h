#pragma once

#include "bus.h"

#include <asio/awaitable.hpp>
#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <span>
#include <string_view>

namespace tt09_levenshtein
{

class Context;

class Client
{
public:
    struct Result
    {
        std::uint16_t index;
        std::uint8_t distance;
    };
    enum class ChipSelect : std::uint8_t
    {
        None = 0,
        CS = 1,
        CS2 = 2,
        CS3 = 3
    };
    
    explicit Client(Context& context, Bus& bus) noexcept;

    constexpr unsigned int maxLength() const noexcept
    {
        return m_maxLength;
    }

    asio::awaitable<void> init(ChipSelect memoryChipSelect, bool clearVectorMap = true);
    
    template<typename Container>
    asio::awaitable<void> loadDictionary(Container&& container)
    {
        constexpr auto wordTerminator = std::to_array<std::uint8_t>({WordTerminator});
        constexpr auto listTerminator = std::to_array<std::uint8_t>({ListTerminator});

        std::uint32_t address = m_dictionaryAddress;
        for (const auto& word : container)
        {
            co_await m_bus.write(address, std::as_bytes(std::span(word)));
            address += word.size();

            co_await m_bus.write(address, std::as_bytes(std::span(wordTerminator)));
            address++;
        }
        
        co_await m_bus.write(address, std::as_bytes(std::span(listTerminator)));
    }

    template<typename Container>
    asio::awaitable<void> verifyDictionary(Container&& container)
    {
        std::vector<std::uint8_t> buffer;

        std::uint32_t address = m_dictionaryAddress;
        for (const auto& word : container)
        {
            buffer.resize(word.size() + 1);
            co_await m_bus.read(address, std::as_writable_bytes(std::span(buffer)));
            for (std::size_t i = 0; i != word.size(); ++i)
            {
                if (buffer[i] != static_cast<std::uint8_t>(word[i]))
                {
                    throw std::runtime_error(fmt::format("Mismatch at address 0x{:06x}. Read {:02x}, expected {:02x}", address + i, buffer[i], static_cast<std::uint8_t>(word[i])).c_str());
                }
            }
            if (buffer.back() != WordTerminator)
            {
                throw std::runtime_error(fmt::format("Mismatch at address 0x{:06x}. Read {:02x}, expected {:02x}", address + word.size(), buffer.back(), static_cast<std::uint8_t>(WordTerminator)).c_str());
            }
            
            address += word.size() + 1;
        }

        buffer.resize(1);
        co_await m_bus.read(address, std::as_writable_bytes(std::span(buffer)));
        if (buffer.front() != ListTerminator)
        {
            throw std::runtime_error(fmt::format("Mismatch at address 0x{:06x}. Read {:02x}, expected {:02x}", address, buffer.front(), static_cast<std::uint8_t>(WordTerminator)).c_str());
        }
    }

    asio::awaitable<Result> search(std::string_view word);

private:
    enum ControlFlags : std::uint8_t
    {
        EnableFlag = 0x01
    };

    enum Address : std::uint32_t
    {
        ControlAddress          = 0x000000,
        SRAMControlAddress      = 0x000001,
        LengthAddress           = 0x000002,
        MaxLengthAddress        = 0x000003,
        IndexAddress            = 0x000004,
        DistanceAddress         = 0x000006
    };

    enum SpecialChars : std::uint8_t
    {
        WordTerminator = 0x00,
        ListTerminator = 0x01
    };

    asio::awaitable<void> writeByte(std::uint32_t address, std::uint8_t value);
    asio::awaitable<void> writeShort(std::uint32_t address, std::uint16_t value);
    asio::awaitable<std::uint8_t> readByte(std::uint32_t address);
    asio::awaitable<std::uint16_t> readShort(std::uint32_t address);

    Context& m_context;
    Bus& m_bus;
    unsigned int m_maxLength = 0;
    unsigned int m_bitvectorSize = 0;
    unsigned int m_bitvectorAlignment = 0;
    std::uint32_t m_vectorMapAddress = 0;
    std::uint32_t m_dictionaryAddress = 0;
};

} // namespace tt09_levenshtein