#pragma once

/**
 * Class to create an awaitable from a callback
 * 
 * Is not a part of the C++23 future
 */

#include <cassert>
#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>

namespace nonstd
{

namespace _make_awaitable
{

class state_base
{
public:
    inline void set_continuation(std::coroutine_handle<> continuation) noexcept
    {
        m_continuation = continuation;
    }

protected:
    inline void resume()
    {
        m_continuation.resume();
    }

private:
    std::coroutine_handle<> m_continuation;
};

template<typename T>
class state final : public state_base
{
public:
    state() noexcept
    {
    }

    state(const state<T>&) = delete;
    state(state<T>&&) = delete;

    ~state()
    {
        if (m_type == result_type::value)
        {
            m_value.~T();
        }
        else if (m_type == result_type::exception)
        {
            m_exception.~exception_ptr();
        }
    }

    state<T>& operator=(const state<T>&) = delete;
    state<T>& operator=(state<T>&&) = delete;

    template<typename U, std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<U>>, T>, bool> = true>
    void set_value(U&& value)
    {
        new(&m_value) T(std::forward<U>(value));
        m_type = result_type::value;
        resume();
    }

    template<typename E, std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, std::exception_ptr>, bool> = true>
    void set_exception(E&& exception)
    {
        new(&m_exception) std::exception_ptr(std::forward<E>(exception));
        m_type = result_type::exception;
        resume();
    }

    T get()
    {
        if (m_type == result_type::exception)
        {
            std::rethrow_exception(m_exception);
        }

        return std::move(m_value);
    }

private:
    enum class result_type
    {
        none,
        value,
        exception
    };
    result_type m_type = result_type::none;
    union
    {
        T m_value;
        std::exception_ptr m_exception;
    };
};

template<typename T>
class state<T&> final : public state_base
{
public:
    constexpr state() noexcept = default;

    state(const state<T&>&) = delete;
    state(state<T&>&&) = delete;

    state<T&>& operator=(const state<T&>&) = delete;
    state<T&>& operator=(state<T&>&&) = delete;

    void set_value(T& value)
    {
        m_value = std::addressof(value);
        resume();
    }

    template<typename E, std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, std::exception_ptr>, bool> = true>
    void set_exception(E&& exception)
    {
        m_exception = std::forward<E>(exception);
        resume();
    }

    T& get()
    {
        if (m_exception)
        {
            std::rethrow_exception(m_exception);
        }

        return *m_value;
    }

private:
    std::exception_ptr m_exception;
    std::add_pointer_t<T> m_value = nullptr;
};

template<>
class state<void> final : public state_base
{
public:
    state() noexcept = default;

    state(const state<void>&) = delete;
    state(state<void>&&) = delete;

    state<void>& operator=(const state<void>&) = delete;
    state<void>& operator=(state<void>&&) = delete;

    inline void set_value()
    {
        resume();
    }

    template<typename E, std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, std::exception_ptr>, bool> = true>
    void set_exception(E&& exception)
    {
        m_exception = std::forward<E>(exception);
        resume();
    }

    inline void get()
    {
        if (m_exception)
        {
            std::rethrow_exception(m_exception);
        }
    }

private:
    std::exception_ptr m_exception;
};

template<typename T>
class promise_base
{
public:
    explicit promise_base(state<T>& state) noexcept
        : m_state(&state)
    {
    }

    promise_base(const promise_base<T>&) = default;

    promise_base<T>& operator=(const promise_base<T>&) = default;

    template<typename E, std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, std::exception_ptr>, bool> = true>
    void set_exception(E&& exception)
    {
        m_state->set_exception(std::forward<E>(exception));
    }

    template<typename E, std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, std::exception_ptr>, bool> = true>
    void set_exception(E&& exception)
    {
        set_exception(std::make_exception_ptr(std::forward<E>(exception)));
    }

    [[nodiscard]] constexpr void* address() const noexcept
    {
        return m_state;
    }

protected:
    state<T>* m_state;
};

template<typename T>
class promise final : public promise_base<T>
{
public:
    explicit promise(state<T>& state) noexcept
        : promise_base<T>(state)
    {
    }

    promise(const promise<T>&) = delete;
    promise(promise<T>&&) noexcept = default;

    promise<T>& operator=(const promise<T>&) = delete;
    promise<T>& operator=(promise<T>&&) noexcept = default;

    void set_value(const T& value)
    {
       this->m_state->set_value(value);
    }

    void set_value(T&& value)
    {
       this->m_state->set_value(std::move(value));
    }

    [[nodiscard]] static promise<T> from_address(void* address) noexcept
    {
        return promise<T>(*static_cast<state<T>*>(address));
    }
};

template<typename T>
class promise<T&> final : public promise_base<T&>
{
public:
    explicit promise(state<T&>& state) noexcept
        : promise_base<T&>(state)
    {
    }

    promise(const promise<T&>&) = delete;
    promise(promise<T&>&&) noexcept = default;

    promise<T&>& operator=(const promise<T&>&) = delete;
    promise<T&>& operator=(promise<T&>&&) noexcept = default;

    void set_value(T& value)
    {
        this->m_state->set_value(value);
    }

    [[nodiscard]] static promise<T&> from_address(void* address) noexcept
    {
        return promise<T&>(*static_cast<state<T&>*>(address));
    }
};

template<>
class promise<void> final : public promise_base<void>
{
public:
    explicit promise(state<void>& state) noexcept
        : promise_base<void>(state)
    {
    }

    promise(const promise<void>&) = delete;
    promise(promise<void>&&) noexcept = default;

    promise<void>& operator=(const promise<void>&) = delete;
    promise<void>& operator=(promise<void>&&) noexcept = default;

    void set_value()
    {
        this->m_state->set_value();
    }

    [[nodiscard]] static promise<void> from_address(void* address) noexcept
    {
        return promise<void>(*static_cast<state<void>*>(address));
    }
};

template<typename T, typename F>
class awaitable
{
public:
    explicit awaitable(F&& func) noexcept(std::is_nothrow_move_constructible_v<F>)
        : m_func(std::move(func))
    {
    }

    awaitable(const awaitable&) = delete;
    awaitable(awaitable&&) = delete;

    awaitable& operator=(const awaitable&) = delete;
    awaitable& operator=(awaitable&&) = delete;

    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> continuation) noexcept(std::is_nothrow_invocable_r_v<void,F,promise<T>>)
    {
        m_state.set_continuation(continuation);
        m_func(promise<T>(m_state));
    }

    T await_resume()
    {
        return std::move(m_state.get());
    }

private:
    state<T> m_state;
    F m_func;
};

template<typename T, typename F>
class awaitable<T&, F>
{
public:
    explicit awaitable(F&& func) noexcept(std::is_nothrow_move_constructible_v<F>)
        : m_func(std::move(func))
    {
    }

    awaitable(const awaitable&) = delete;
    awaitable(awaitable&&) = delete;

    awaitable& operator=(const awaitable&) = delete;
    awaitable& operator=(awaitable&&) = delete;

    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> continuation) noexcept(std::is_nothrow_invocable_r_v<void,F,promise<T&>>)
    {
        m_state.set_continuation(continuation);
        m_func(promise<T&>(m_state));
    }

    T& await_resume()
    {
        return m_state.get();
    }

private:
    state<T&> m_state;
    F m_func;
};

template<typename F>
class awaitable<void,F>
{
public:
    explicit awaitable(F&& func) noexcept(std::is_nothrow_move_constructible_v<F>)
        : m_func(std::move(func))
    {
    }

    awaitable(const awaitable&) = delete;
    awaitable(awaitable&&) = delete;

    awaitable& operator=(const awaitable&) = delete;
    awaitable& operator=(awaitable&&) = delete;

    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> continuation) noexcept(std::is_nothrow_invocable_r_v<void,F,promise<void>>)
    {
        m_state.set_continuation(continuation);
        m_func(promise<void>(m_state));
    }

    void await_resume()
    {
        m_state.get();
    }

private:
    state<void> m_state;
    F m_func;
};

} // namespace _make_awaitable

template<typename T>
using awaitable_promise = _make_awaitable::promise<T>;

template<typename T, typename F, std::enable_if_t<std::is_invocable_r_v<void,F&&,awaitable_promise<T>>, bool> = true>
auto make_awaitable(F&& func) noexcept(std::is_nothrow_move_constructible_v<F>)
{
    return _make_awaitable::awaitable<T,F>(std::move(func));
}

} // namespace nonstd