#include "real_context.h"

#include <asio/steady_timer.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>

namespace tt09_levenshtein
{

asio::awaitable<void> RealContext::init()
{
    co_return;
}

asio::awaitable<void> RealContext::wait(std::chrono::nanoseconds time)
{
    auto executor = co_await asio::this_coro::executor;

    asio::steady_timer timer(executor);
    timer.expires_after(time);
    co_await timer.async_wait(asio::use_awaitable);
}

} // namespace tt09_levenshtein