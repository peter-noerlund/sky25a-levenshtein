#include "spi_bus.h"

#include "spi.h"

#include <fmt/format.h>
#ifdef SPI_BUS_DEBUG
#include <fmt/printf.h>
#endif // SPI_BUS_DEBUG

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <stdexcept>

namespace tt09_levenshtein
{

SpiBus::SpiBus(Spi& spi) noexcept
    : m_spi(spi)
{
}

asio::awaitable<std::byte> SpiBus::execute(std::uint32_t command)
{
    std::exception_ptr exception;

    co_await m_spi.enable();
    
    std::byte response = {};
    try
    {
        std::array<std::byte, 4> data = {
            static_cast<std::byte>(command >> 24),
            static_cast<std::byte>(command >> 16),
            static_cast<std::byte>(command >> 8),
            static_cast<std::byte>(command),
        };
        std::array<std::byte, 4> buffer;

#ifdef SPI_BUS_DEBUG
        fmt::print("\033[33m{:02x} {:02x} {:02x} {:02x}\033[0m", std::to_integer<std::uint8_t>(buffer[0]), std::to_integer<std::uint8_t>(buffer[1]), std::to_integer<std::uint8_t>(buffer[2]), std::to_integer<std::uint8_t>(buffer[3]));
#endif // SPI_BUS_DEBUG

        co_await m_spi.xmit(data, buffer);

        auto syncIt = buffer.end();
        for (unsigned int retries = 0; retries != 256; ++retries)
        {
            for (auto it = buffer.begin(); it != buffer.end(); ++it)
            {
                auto value = std::to_integer<std::uint8_t>(*it);
                if (value != 0)
                {
                    syncIt = it;
                    break;
                }
#ifdef SPI_BUS_DEBUG
                else
                {
                    fmt::print("\033[34m {:02x}\033[0m", value);
                }
#endif // SPI_BUS_DEBUG
            }

            if (syncIt != buffer.end())
            {
                break;
            }

            if (retries < 255)
            {
                co_await m_spi.xmit({}, buffer);
            }
        }

        if (syncIt == buffer.end())
        {
            throw std::runtime_error("SPI Timeout");
        }

        std::uint16_t value = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(*syncIt++)) << 8;
        if (syncIt == buffer.end())
        {
            co_await m_spi.xmit({}, std::span(buffer).subspan(0, 1));
            syncIt = buffer.begin();
        }

        value |= std::to_integer<std::uint8_t>(*syncIt);

        for (unsigned int i = 0; i != 8; ++i)
        {
            if (value >> (i + 8) == 1)
            {
                response = std::byte(static_cast<std::uint8_t>(value >> i));
                break;
            }
        }
#ifdef SPI_BUS_DEBUG
        fmt::println("\033[32m {:02x} {:02x}\033[0m ({:02x})", value >> 8, value & 0xFF, std::to_integer<std::uint8_t>(response));
#endif // SPI_BUS_DEBUG
    }
    catch (...)
    {
        exception = std::current_exception();
    }

    co_await m_spi.disable();

    if (exception)
    {
        std::rethrow_exception(exception);
    }

    co_return response;
}


} // namespace tt09_levenshtein