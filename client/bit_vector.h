#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace tt09_levenshtein
{

class BitVector
{
public:
    constexpr BitVector() noexcept = default;

    constexpr BitVector(unsigned int size)
        : m_size(size)
    {
        if (size > m_data.size() * 8)
        {
            throw std::out_of_range("Unsupported size");
        }
        if (size % 8)
        {
            throw std::invalid_argument("Size must be modulo 8");
        }
    }

    constexpr void set(unsigned int bit)
    {
        if (bit >= m_size)
        {
            throw std::out_of_range("Invalid bit");
        }

        auto byte = (m_size - 1 - bit) / 8;
        auto byteBit = bit % 8;
        
        m_data[byte] |= (1 << byteBit);
    }

    constexpr std::span<const std::uint8_t> data() const noexcept
    {
        return std::span(m_data).subspan(0, m_size / 8);
    }

private:
    std::array<std::uint8_t, 32> m_data = {};
    unsigned int m_size;
};

} // namespace tt09_levenshtein