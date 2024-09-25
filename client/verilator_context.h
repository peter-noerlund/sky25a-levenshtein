#pragma once

#include "context.h"
#include "Vtop.h"

#include <asio/awaitable.hpp>
#include <asio/post.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>
#include <verilated_vcd_c.h>

#include <chrono>
#include <memory>
#include <filesystem>
#include <optional>

namespace tt09_levenshtein
{

class VerilatorContext : public Context
{
public:
    explicit VerilatorContext(unsigned long int frequency);
    VerilatorContext(unsigned long int frequency, const std::filesystem::path& vcdFileName);

    template<typename T, typename V = int>
    asio::awaitable<void> fallingEdge(T& pin, V mask = 1)
    {
        auto executor = co_await asio::this_coro::executor;

        auto oldValue = pin & mask;
        while (true)
        {
            co_await asio::post(executor, asio::use_awaitable);
            auto newValue = pin & mask;
            if (oldValue && !newValue)
            {
                break;
            }
            oldValue = newValue;
        }
    }

    template<typename T, typename V = int>
    asio::awaitable<void> risingEdge(T& pin, V mask = 1)
    {
        auto executor = co_await asio::this_coro::executor;

        auto oldValue = pin & mask;
        while (true)
        {
            co_await asio::post(executor, asio::use_awaitable);
            auto newValue = pin & mask;
            if (!oldValue && newValue)
            {
                break;
            }
            oldValue = newValue;
        }
    }

    asio::awaitable<void> clock();

    asio::awaitable<void> clocks(unsigned int count);

    asio::awaitable<void> init() override;
    asio::awaitable<void> wait(std::chrono::nanoseconds time) override;

    constexpr Vtop& top() noexcept
    {
        return m_top;
    }

    constexpr const Vtop& top() const noexcept
    {
        return m_top;
    }

private:
    asio::awaitable<void> runClock();
    
    std::chrono::nanoseconds m_halfPeriod;
    std::chrono::nanoseconds m_time = {};
    std::unique_ptr<VerilatedVcdC> m_vcd;
    Vtop m_top;
};

} // namespace tt09_levenshtein
