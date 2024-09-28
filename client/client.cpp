#include "client.h"

#include "bus.h"
#include "context.h"

#include <fmt/format.h>

#include <cstdint>
#include <map>
#include <stdexcept>

namespace tt09_levenshtein
{

Client::Client(Context& context, Bus& bus) noexcept
    : m_context(context)
    , m_bus(bus)
{
}

asio::awaitable<void> Client::init()
{
    for (unsigned int i = 0; i != 256; ++i)
    {
        co_await writeShort(BaseBitvectorAddress + i * 2, 0);
    }
}

asio::awaitable<Client::Result> Client::search(std::string_view word)
{
    if (word.size() > 16)
    {
        throw std::invalid_argument(fmt::format("Word \"{}\" exceeds 16 characters", word).c_str());
    }

    // Verify accelerator is idle

    auto ctrl = co_await readByte(ControlAddress);
    if ((ctrl & ActiveFlag) != 0)
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

    for (auto [c, vector] : vectorMap)
    {
        co_await writeShort(BaseBitvectorAddress + static_cast<std::uint32_t>(c) * 2, vector);
    }

    // Initiate search

    co_await writeByte(ControlAddress, word.size());

    while (true)
    {
        co_await m_context.wait(std::chrono::microseconds(10));

        ctrl = co_await readByte(ControlAddress);
        if ((ctrl & ActiveFlag) == 0)
        {
            break;
        }
    }

    Result result;
    result.distance = co_await readByte(DistanceAddress);
    result.index = co_await readShort(IndexAddress);

    // Clear bitvectors
    for (auto [c, vector] : vectorMap)
    {
        co_await writeShort(BaseBitvectorAddress + static_cast<std::uint32_t>(c) * 2, 0);
    }

    co_return result;
}

asio::awaitable<void> Client::writeByte(std::uint32_t address, std::uint8_t value)
{
    auto data = std::to_array<std::uint8_t>({value});
    co_await m_bus.write(address, std::as_bytes(std::span(data)));
}

asio::awaitable<std::uint8_t> Client::readByte(std::uint32_t address)
{
    std::array<std::byte, 1> buffer;
    co_await m_bus.read(address, buffer);
    co_return std::to_integer<std::uint8_t>(buffer.front());
}

asio::awaitable<void> Client::writeShort(std::uint32_t address, std::uint16_t value)
{
    auto data = std::to_array<std::uint8_t>({
        static_cast<std::uint8_t>(value >> 8),
        static_cast<std::uint8_t>(value)
    });
    co_await m_bus.write(address, std::as_bytes(std::span(data)));
}

asio::awaitable<std::uint16_t> Client::readShort(std::uint32_t address)
{
    std::array<std::byte, 2> buffer;
    co_await m_bus.read(address, buffer);

    co_return 
        (std::to_integer<std::uint16_t>(buffer[0]) << 8) |
        (std::to_integer<std::uint8_t>(buffer[1]));
}

} // namespace tt09_levenshtein