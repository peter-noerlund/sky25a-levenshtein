#pragma once

#include "basic_bus.h"
#include "uart.h"

namespace tt09_levenshtein
{

class UartBus : public BasicBus
{
public:
    UartBus(Uart& uart) noexcept;

protected:
    asio::awaitable<std::byte> execute(std::uint32_t command);

    Uart& m_uart;
};

} // namespace tt09_levenshtein