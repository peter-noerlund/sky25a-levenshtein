#pragma once

#include "nonstd/lazy.h"

#include <type_traits>
#include <utility>

namespace nonstd
{
namespace _then
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

} // namespace _then

template<typename F>
_then::adapter<F> then(F&& func)
{
    return _then::adapter(std::forward<F>(func));
}

} // namespace nonstd

template<typename T, typename F, std::enable_if_t<!std::is_void_v<T>, bool> = true>
nonstd::lazy<std::invoke_result_t<F, T>> operator|(nonstd::lazy<T> task, nonstd::_then::adapter<F> adapter)
{
    co_return adapter(co_await task);
}

template<typename F>
nonstd::lazy<std::invoke_result_t<F>> operator|(nonstd::lazy<void> task, nonstd::_then::adapter<F> adapter)
{
    co_await task;
    co_return adapter();
}


