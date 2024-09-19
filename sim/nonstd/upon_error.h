#pragma once

#include "nonstd/lazy.h"

#include <exception>
#include <type_traits>

namespace nonstd
{
namespace _upon_error
{

template<typename F>
class adapter
{
public:
    explicit adapter(F&& func) noexcept(noexcept(std::is_nothrow_move_constructible_v<F>))
        : m_func(std::move(func))
    {
    }

    adapter(const adapter&) = default;
    adapter(adapter&&) = default;

    adapter& operator=(const adapter&) = default;
    adapter& operator=(adapter&&) = default;

    template<typename... ArgTypes>
    std::invoke_result_t<F, ArgTypes...> operator()(ArgTypes&&... args) noexcept(noexcept(std::is_nothrow_invocable_v<F, ArgTypes...>))
    {
        return m_func(std::forward<ArgTypes>(args)...);
    }

private:
    F m_func;
};

} // namespace _upon_error

template<typename F>
_upon_error::adapter<F> upon_error(F&& func)
{
    return _upon_error::adapter(std::forward<F>(func));
}

} // namespace nonstd

template<typename T, typename F, std::enable_if_t<!std::is_void_v<T>, bool> = true>
nonstd::lazy<T> operator|(nonstd::lazy<T> task, nonstd::_upon_error::adapter<F> adapter)
{
    try
    {
        co_return co_await task;
    }
    catch (...)
    {
        co_return adapter(std::current_exception());
    }
}


template<typename F>
nonstd::lazy<void> operator|(nonstd::lazy<void> task, nonstd::_upon_error::adapter<F> adapter)
{
    try
    {
        co_await task;
        co_return;
    }
    catch (...)
    {
        co_return adapter(std::current_exception());
    }
}