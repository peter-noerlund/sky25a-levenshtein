#pragma once

#include "basic_bus.h"

namespace tt09_levenshtein
{

class Spi;

class SpiBus : public BasicBus
{
public:
    explicit SpiBus(Spi& spi) noexcept;

protected:
    asio::awaitable<std::byte> execute(std::uint32_t command) override;

private:
    Spi& m_spi;
};

} // namespace tt09_levenshtein