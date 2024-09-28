#pragma once

#include "basic_bus.h"
#include "context.h"

#include <ftdi.h>

namespace tt09_levenshtein
{

class IcestickSpiBus : public BasicBus
{
public:
    explicit IcestickSpiBus(Context& context, unsigned int clockDivider = 4);
    ~IcestickSpiBus();

protected:
    asio::awaitable<std::byte> execute(std::uint32_t command);

private:
    asio::awaitable<void> init();
    void sendCommands(std::span<const std::uint8_t> commands);
    void readResponse(std::span<std::uint8_t> data);

    Context& m_context;
    ftdi_context* m_device;
    bool m_initialized = false;
};

} // namespace tt09_levenshtein