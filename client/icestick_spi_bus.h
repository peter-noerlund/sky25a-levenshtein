#pragma once

#include "basic_bus.h"
#include "context.h"

#include <ftdi.h>

namespace tt09_levenshtein
{

class IcestickSpiBus : public BasicBus
{
public:
    explicit IcestickSpiBus(Context& context, unsigned int clockDivider = 6);
    ~IcestickSpiBus();

protected:
    asio::awaitable<std::byte> execute(std::uint32_t command) override;

private:
    enum Pin : std::uint8_t
    {
        SCK = 1 << 0,
        MOSI = 1 << 1,
        MISO = 1 << 2,
        SS = 1 << 3
    };

    asio::awaitable<void> init();
    void sendCommands(std::span<const std::uint8_t> commands);
    void readResponse(std::span<std::uint8_t> data);

    Context& m_context;
    ftdi_context* m_device;
    unsigned int m_clockDivider;
    bool m_initialized = false;
};

} // namespace tt09_levenshtein