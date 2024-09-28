#pragma once

#include "bus.h"

namespace tt09_levenshtein
{

class VerilatorContext;

class VerilatorSpiBus : public Bus
{
public:
    explicit VerilatorSpiBus(VerilatorContext& context, unsigned int clockDivider = 4) noexcept;

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
    
    VerilatorContext& m_context;
    unsigned int m_clockDivider;
};

} // namespace tt09_levenshtein