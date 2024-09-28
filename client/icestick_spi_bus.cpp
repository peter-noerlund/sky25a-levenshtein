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
    ftdi_set_bitmode(m_device, 0, 0);
    ftdi_set_bitmode(m_device, 0, BITMODE_MPSSE);
    ftdi_usb_purge_buffers(m_device);

    co_await m_context.wait(std::chrono::milliseconds(50));

    m_initialized = true;
}

asio::awaitable<std::byte> IcestickSpiBus::execute(std::uint32_t command)
{
    enum Pin : std::uint8_t
    {
        SCK = 1 << 0,
        MOSI = 1 << 1,
        MISO = 1 << 2,
        SS = 1 << 3
    };

    auto executor = co_await asio::this_coro::executor;

    co_await init();

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
        MPSSE_DO_READ | MPSSE_BITMODE | MPSSE_READ_NEG, 0, 0
    });

    std::array<std::uint8_t, 1> buffer;
    for (unsigned int i = 0; i != 10000; ++i)
    {
        sendCommands(pollCommands);
        readResponse(buffer);
        if (buffer.front())
        {
            break;
        }

        co_await asio::post(executor, asio::use_awaitable);
    }

    if (!buffer.front())
    {
        throw std::runtime_error("SPI error");
    }

    auto readCommands = std::to_array<std::uint8_t>({
        MPSSE_DO_READ | MPSSE_READ_NEG, 0, 0,
        SET_BITS_LOW, Pin::SS, Pin::SCK | Pin::MOSI | Pin::SS,
        CLK_BITS, 0
    });

    sendCommands(readCommands);
    readResponse(buffer);

    co_return std::byte{buffer.front()};
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