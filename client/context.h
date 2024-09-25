#pragma once

#include <asio/awaitable.hpp>

#include <chrono>

namespace tt09_levenshtein
{

class Context
{
public:
    virtual ~Context() = default;

    virtual asio::awaitable<void> init() = 0;
    virtual asio::awaitable<void> wait(std::chrono::nanoseconds time) = 0;
};

} // namespace tt09_levenshtein