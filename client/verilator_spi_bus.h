#pragma once

#include "basic_bus.h"

namespace tt09_levenshtein
{

class VerilatorContext;

class VerilatorSpiBus : public BasicBus
{
public:
    explicit VerilatorSpiBus(VerilatorContext& context, unsigned int clockDivider = 4) noexcept;

protected:
    asio::awaitable<std::byte> execute(std::uint32_t command);
    
    VerilatorContext& m_context;
    unsigned int m_clockDivider;
};

} // namespace tt09_levenshtein