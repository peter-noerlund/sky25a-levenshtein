#include "client.h"

#include "bit_vector.h"
#include "bus.h"
#include "context.h"

#include <fmt/format.h>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

namespace tt09_levenshtein
{

Client::Client(Context& context, Bus& bus) noexcept
    : m_context(context)
    , m_bus(bus)
{
}

asio::awaitable<void> Client::init(ChipSelect memoryChipSelect, bool clearVectorMap)
{
    m_maxLength = static_cast<unsigned int>(co_await readByte(MaxLengthAddress)) + 1;
    m_bitvectorSize = ((m_maxLength + 7) / 8) * 8;
    if (m_bitvectorSize > 128)
    {
        m_bitvectorAlignment = 32;
    }
    else if (m_bitvectorSize > 64)
    {
        m_bitvectorAlignment = 16;
    }
    else if (m_bitvectorSize > 32)
    {
        m_bitvectorAlignment = 8;
    }
    else if (m_bitvectorSize > 16)
    {
        m_bitvectorAlignment = 4;
    }
    else if (m_bitvectorSize > 8)
    {
        m_bitvectorAlignment = 2;
    }
    else
    {
        m_bitvectorAlignment = 1;
    }
    m_vectorMapAddress = 256 * m_bitvectorAlignment;
    m_dictionaryAddress = m_vectorMapAddress * 2;
  
    co_await writeByte(SRAMControlAddress, static_cast<std::uint8_t>(memoryChipSelect));

    if (clearVectorMap)
    {
        for (unsigned int i = 0; i != 256; ++i)
        {
            for (unsigned int j = 0; j != m_bitvectorSize / 8; ++j)
            {
                co_await writeByte(m_vectorMapAddress + i * m_bitvectorAlignment + j, 0);
            }
        }
    }
}

asio::awaitable<Client::Result> Client::search(std::string_view word)
{
    if (word.size() > m_maxLength)
    {
        throw std::invalid_argument(fmt::format("Word \"{}\" exceeds {} characters", word, m_maxLength));
    }
    if (word.empty())
    {
        throw std::invalid_argument("Word is empty");
    }

    // Verify accelerator is idle

    auto ctrl = co_await readByte(ControlAddress);
    if ((ctrl & EnableFlag) != 0)
    {
        throw std::runtime_error("Cannot search while another search is in progress");
    }

    // Write length

    co_await writeByte(LengthAddress, word.size() - 1);

    // Generate bitvectors

    std::map<std::uint8_t, BitVector> vectorMap;
    for (auto c : word)
    {
        if (vectorMap.contains(static_cast<std::uint8_t>(c)))
        {
            continue;
        }

        BitVector vector(m_bitvectorSize);
        for (std::string_view::size_type i = 0; i != word.size(); ++i)
        {
            if (word[i] == c)
            {
                vector.set(i);
            }
        }
        vectorMap[static_cast<std::uint8_t>(c)] = vector;
    }

    // Write bitvectors

    for (auto [c, vector] : vectorMap)
    {
        co_await m_bus.write(m_vectorMapAddress + static_cast<std::uint32_t>(c) * m_bitvectorAlignment, std::as_bytes(vector.data()));
    }

    // Initiate search

    co_await writeByte(ControlAddress, EnableFlag);

    while (true)
    {
        co_await m_context.wait(std::chrono::microseconds(10));

        ctrl = co_await readByte(ControlAddress);
        if ((ctrl & EnableFlag) == 0)
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
        co_await m_bus.write(m_vectorMapAddress + static_cast<std::uint32_t>(c) * m_bitvectorAlignment, std::as_bytes(BitVector(m_bitvectorSize).data()));
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