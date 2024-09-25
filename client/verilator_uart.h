#pragma once

#include "uart.h"

namespace tt09_levenshtein
{

class VerilatorContext;

class VerilatorUart : public Uart
{
public:
    explicit VerilatorUart(VerilatorContext& context, unsigned int clockDivider = 16) noexcept;

    asio::awaitable<void> send(std::span<const std::byte> data) override;
    asio::awaitable<void> recv(std::span<std::byte> buffer) override;

private:
    asio::awaitable<void> send(std::byte value);
    asio::awaitable<std::byte> recv();

    VerilatorContext& m_context;
    unsigned int m_clockDivider;
};

} // namespace tt09_levenshtein