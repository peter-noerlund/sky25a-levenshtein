#include "icestick_spi.h"

#include <asio/post.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>

#include <stdexcept>

namespace tt09_levenshtein
{

std::uint8_t IcestickSpi::s_outputPins = IcestickSpi::Pin::SS | IcestickSpi::Pin::MOSI | IcestickSpi::Pin::SCK;

IcestickSpi::IcestickSpi(Context& context, unsigned int clockDivider)
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

IcestickSpi::~IcestickSpi()
{
    if (m_device)
    {
        ftdi_free(m_device);
    }
}

asio::awaitable<void> IcestickSpi::init()
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
        TCK_DIVISOR, 2, 0, // 10MHz
        DIS_DIV_5,
        DIS_ADAPTIVE,
        DIS_3_PHASE,
        SET_BITS_LOW, Pin::SS, s_outputPins
    });
    send(commands);

    m_initialized = true;
}

asio::awaitable<void> IcestickSpi::enable()
{
    co_await init();
    co_await setSS(false);
}

asio::awaitable<void> IcestickSpi::disable()
{
    co_await setSS(true);
}

asio::awaitable<void> IcestickSpi::setSS(bool high)
{
    auto commands = std::to_array<std::uint8_t>({
        SET_BITS_LOW, high ? static_cast<std::uint8_t>(Pin::SS) : std::uint8_t(0), s_outputPins
    });
    send(commands);
    co_return;
}

asio::awaitable<void> IcestickSpi::xmit(std::span<const std::byte> data, std::span<std::byte> buffer)
{
    std::vector<std::uint8_t> commands;
    commands.reserve(7 + data.size() + buffer.size());

    if (!data.empty())
    {
        commands.push_back(MPSSE_DO_WRITE | MPSSE_WRITE_NEG);
        commands.push_back(static_cast<std::uint8_t>(data.size() - 1));
        commands.push_back(static_cast<std::uint8_t>((data.size() - 1) >> 8));
        for (auto b : data)
        {
            commands.push_back(static_cast<std::uint8_t>(b));
        }
    }

    if (!buffer.empty())
    {
        commands.push_back(MPSSE_DO_READ);
        commands.push_back(static_cast<std::uint8_t>(buffer.size() - 1));
        commands.push_back(static_cast<std::uint8_t>((buffer.size() - 1) >> 8));
        commands.push_back(SEND_IMMEDIATE);
    }

    send(commands);
    if (!buffer.empty())
    {
        recv(buffer);
    }

    co_return;
}

void IcestickSpi::send(std::span<const std::uint8_t> commands)
{
    auto res = ftdi_write_data(m_device, const_cast<std::uint8_t*>(commands.data()), commands.size());
    if (res != static_cast<int>(commands.size()))
    {
        throw std::runtime_error("Device write error");
    }
}

void IcestickSpi::recv(std::span<std::byte> buffer)
{
    for (unsigned int retries = 0; retries != 16; ++retries)
    {
        auto res = ftdi_read_data(m_device, reinterpret_cast<std::uint8_t*>(buffer.data()), buffer.size());

        buffer = buffer.subspan(res);
        if (buffer.empty())
        {
            return;
        }
    }

    throw std::runtime_error("Device read error");
}

} // namespace tt09_levenshtein