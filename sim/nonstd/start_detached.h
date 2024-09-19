#pragma once

#include "nonstd/lazy.h"

#include <coroutine>
#include <exception>

namespace nonstd
{

namespace _start_detached
{

struct void_type
{
    struct promise_type
    {
        inline void_type get_return_object() noexcept
        {
            return {};
        }

        inline std::suspend_never initial_suspend() noexcept
        {
            return {};
        }

        inline void return_void() noexcept
        {
        }

        inline void unhandled_exception() noexcept
        {
            std::terminate();
        }

        inline std::suspend_never final_suspend() noexcept
        {
            return {};
        }
    };
};

} // namespace _start_detached

template<typename T>
_start_detached::void_type start_detached(lazy<T> task) noexcept
{
    co_await task;
}

} // namepace nonstd