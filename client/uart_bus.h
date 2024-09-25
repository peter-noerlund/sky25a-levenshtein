#pragma once

#include "bus.h"
#include "uart.h"

namespace tt09_levenshtein
{

class UartBus : public Bus
{
public:
    UartBus(Uart& uart) noexcept;

    asio::awaitable<void> read(std::uint32_t address, std::span<std::byte> buffer) override;
    asio::awaitable<void> write(std::uint32_t address, std::span<const std::byte> data) override;

private:
    enum class Operation
    {
        Read,
        Write
    };

    asio::awaitable<std::byte> read(std::uint32_t address);
    asio::awaitable<void> write(std::uint32_t address, std::byte value);
    asio::awaitable<std::byte> execute(Operation operation, std::uint32_t address, std::byte value = {});

    Uart& m_uart;
};

} // namespace tt09_levenshtein