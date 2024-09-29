#pragma once

#include "context.h"
#include "spi.h"

#include <ftdi.h>

namespace tt09_levenshtein
{

class IcestickSpi : public Spi
{
public:
    explicit IcestickSpi(Context& context, unsigned int clockDivider = 6);
    ~IcestickSpi();

public:
    asio::awaitable<void> enable() override;
    asio::awaitable<void> xmit(std::span<const std::byte> data, std::span<std::byte> buffer) override;
    asio::awaitable<void> disable() override;

private:
    enum Pin : std::uint8_t
    {
        SCK = 1 << 0,
        MOSI = 1 << 1,
        MISO = 1 << 2,
        SS = 1 << 3
    };

    asio::awaitable<void> init();
    void send(std::span<const std::uint8_t> commands);
    void recv(std::span<std::byte> data);
    asio::awaitable<void> setSS(bool high);

    Context& m_context;
    ftdi_context* m_device;
    unsigned int m_clockDivider;
    bool m_initialized = false;
    static std::uint8_t s_outputPins;
};

} // namespace tt09_levenshtein