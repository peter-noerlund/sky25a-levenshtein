#pragma once

#include "spi.h"

namespace tt09_levenshtein
{

class VerilatorContext;

class VerilatorSpi : public Spi
{
public:
    explicit VerilatorSpi(VerilatorContext& context, unsigned int clockDivider = 4) noexcept;

    asio::awaitable<void> enable() override;
    asio::awaitable<void> xmit(std::span<const std::byte> data, std::span<std::byte> buffer) override;
    asio::awaitable<void> disable() override;
    
private:
    VerilatorContext& m_context;
    unsigned int m_clockDivider;
};

} // namespace tt09_levenshtein