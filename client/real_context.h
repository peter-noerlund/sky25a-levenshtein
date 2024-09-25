#pragma once

#include "context.h"

namespace tt09_levenshtein
{

class RealContext : public Context
{
public:
    asio::awaitable<void> init() override;
    asio::awaitable<void> wait(std::chrono::nanoseconds time) override;
};

} // namespace tt09_levenshtein