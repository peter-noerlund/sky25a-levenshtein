#include "icestick_spi_bus.h"

#include <asio/post.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>

#include <stdexcept>

namespace tt09_levenshtein
{

IcestickSpiBus::IcestickSpiBus(Context& context, unsigned int clockDivider)
    : m_context(context)
    , m_device(ftdi_new())
    , m_clockDivider(clockDivider)
{
    ftdi_set_interface(m_device, INTERFACE_B);

    if (ftdi_usb_open(m_device, 0x0403, 0x6010))
    {
        throw std::runtime_error("Error opening device");
    }
}

IcestickSpiBus::~IcestickSpiBus()
{
    if (m_device)
    {
        ftdi_free(m_device);
    }
}

asio::awaitable<void> IcestickSpiBus::init()
{
    if (m_initialized)
    {
        co_return;
    }

    ftdi_usb_reset(m_device);
    ftdi_set_bitmode(m_device, 0, BITMODE_RESET);
    ftdi_set_bitmode(m_device, 0, BITMODE_MPSSE);

    co_await m_context.wait(std::chrono::milliseconds(50));

    auto commands = std::to_array<std::uint8_t>({
        TCK_DIVISOR, 0, 2, // 10MHz
        DIS_DIV_5,
        DIS_ADAPTIVE,
        DIS_3_PHASE,
        SET_BITS_LOW, Pin::SS, Pin::SCK | Pin::MOSI | Pin::SS,
        GET_BITS_LOW

    });
    sendCommands(commands);
    std::array<std::uint8_t, 1> buffer;
    readResponse(buffer);

    m_initialized = true;
}

asio::awaitable<std::byte> IcestickSpiBus::execute(std::uint32_t command)
{
    auto executor = co_await asio::this_coro::executor;

    co_await init();

    try
    {
        auto writeCommands = std::to_array<std::uint8_t>({
            SET_BITS_LOW, 0, Pin::SCK | Pin::MOSI | Pin::SS,

            MPSSE_DO_WRITE | MPSSE_WRITE_NEG, 3, 0,
            static_cast<std::uint8_t>(command >> 24),
            static_cast<std::uint8_t>(command >> 16),
            static_cast<std::uint8_t>(command >> 8),
            static_cast<std::uint8_t>(command)
        });
        sendCommands(writeCommands);

        auto pollCommands = std::to_array<std::uint8_t>({
            MPSSE_DO_READ | MPSSE_BITMODE, 0,
            SEND_IMMEDIATE
        });

        std::array<std::uint8_t, 1> buffer;
        for (unsigned int i = 0; i != 1000; ++i)
        {
            sendCommands(pollCommands);
            readResponse(buffer);

            if (buffer.front() != 0)
            {
                break;
            }

            co_await asio::post(executor, asio::use_awaitable);
        }

        if (buffer.front() == 0)
        {
            throw std::runtime_error("SPI error");
        }

        auto readCommands = std::to_array<std::uint8_t>({
            MPSSE_DO_READ, 0, 0,
            SET_BITS_LOW, Pin::SS, Pin::SCK | Pin::MOSI | Pin::SS,
            SEND_IMMEDIATE
        });
        
        sendCommands(readCommands);
        readResponse(buffer);

        co_return std::byte(buffer.front());
    }
    catch (...)
    {
        auto finalCommands = std::to_array<std::uint8_t>({
            SET_BITS_LOW, Pin::SS, Pin::SCK | Pin::MOSI | Pin::SS
        });
        sendCommands(finalCommands);
        throw;
    }

}

void IcestickSpiBus::sendCommands(std::span<const std::uint8_t> commands)
{
    auto res = ftdi_write_data(m_device, const_cast<std::uint8_t*>(commands.data()), commands.size());
    if (res != static_cast<int>(commands.size()))
    {
        throw std::runtime_error("Device write error");
    }
}

void IcestickSpiBus::readResponse(std::span<std::uint8_t> buffer)
{
    auto res = ftdi_read_data(m_device, buffer.data(), buffer.size());
    if (res != static_cast<int>(buffer.size()))
    {
        throw std::runtime_error("Device read error");
    }
}

} // namespace tt09_levenshtein