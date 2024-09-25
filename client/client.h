#pragma once

#include "bus.h"

#include <asio/awaitable.hpp>

#include <cstddef>
#include <cstdint>
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
    
    explicit Client(Context& context, Bus& bus) noexcept;

    asio::awaitable<void> init();
    
    template<typename Container>
    asio::awaitable<void> loadDictionary(Container&& container)
    {
        constexpr auto wordTerminator = std::to_array<std::uint8_t>({WordTerminator});
        constexpr auto listTerminator = std::to_array<std::uint8_t>({ListTerminator});

        std::uint32_t address = BaseDictionaryAddress;
        for (const auto& word : container)
        {
            co_await m_bus.write(address, std::as_bytes(std::span(word)));
            address += word.size();

            co_await m_bus.write(address, std::as_bytes(std::span(wordTerminator)));
            address++;
        }
        
        co_await m_bus.write(address, std::as_bytes(std::span(listTerminator)));
    }

    asio::awaitable<Result> search(std::string_view word);

private:
    enum ControlFlags : std::uint8_t
    {
        EnableFlag = 0x01
    };

    enum Address : std::uint32_t
    {
        ControlAddress = 0x0000000,
        LengthAddress = 0x000001,
        MaskAddress = 0x000002,
        VpAddress = 0x000004,
        DistanceAddress = 0x0000006,
        IndexAddress = 0x0000008,
        BaseBitvectorAddress = 0x400000,
        BaseDictionaryAddress = 0x600000
    };

    enum SpecialChars : std::uint8_t
    {
        WordTerminator = 0xFE,
        ListTerminator = 0xFF
    };

    asio::awaitable<void> writeByte(std::uint32_t address, std::uint8_t value);
    asio::awaitable<void> writeShort(std::uint32_t address, std::uint16_t value);
    asio::awaitable<std::uint8_t> readByte(std::uint32_t address);
    asio::awaitable<std::uint16_t> readShort(std::uint32_t address);

    Context& m_context;
    Bus& m_bus;
};

} // namespace tt09_levenshtein