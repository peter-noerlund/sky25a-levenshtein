#pragma once

/**
 * Implementation of http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1056r1.html (A lazy coroutine (coroutine task) type)
 * 
 * The following has not been implemented:
 *   - Custom allocator with allocator_arg_t
 *   - Distinction between co_await()& and co_await()&&
 */

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>

namespace nonstd
{

namespace _lazy
{

template<typename T>
class lazy;

template<typename T>
class promise;

template<typename T>
class final_awaitable;

template<typename T>
class promise_base
{
public:
    std::suspend_always initial_suspend() noexcept
    {
        return {};
    }

    final_awaitable<T> final_suspend() noexcept;

    void set_continuation(std::coroutine_handle<> continuation) noexcept
    {
        m_continuation = continuation;
    }

    std::coroutine_handle<> continuation() const noexcept
    {
        return m_continuation;
    }

private:
    std::coroutine_handle<> m_continuation;
};

template<typename T>
class promise final : public promise_base<T>
{
public:
    promise() noexcept
    {
    }

    promise(const promise<T>&) = delete;
    promise(promise<T>&&) = delete;

    ~promise()
    {
        if (m_type == result_type::exception)
        {
            m_exception.~exception_ptr();
        }
        else if (m_type == result_type::value)
        {
            m_value.~T();
        }
    }

    promise<T>& operator=(const promise<T>&) = delete;
    promise<T>& operator=(promise<T>&&) = delete;

    lazy<T> get_return_object();

    void return_value(const T& value)
    {
        new(&m_value) T(value);
        m_type = result_type::value;
    }

    void return_value(T&& value)
    {
        new(&m_value) T(std::move(value));
        m_type = result_type::value;
    }

    void unhandled_exception()
    {
        new(&m_exception) std::exception_ptr(std::move(std::current_exception()));
        m_type = result_type::exception;
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
class promise<T&> final : public promise_base<T&>
{
public:
    promise() noexcept = default;
    promise(const promise<T&>&) = delete;
    promise(promise<T&>&&) = delete;

    promise<T&>& operator=(const promise<T&>&) = delete;
    promise<T&>& operator=(promise<T&>&&) = delete;

    lazy<T&> get_return_object();

    void return_value(T& value)
    {
        m_value = std::addressof(value);
    }

    void unhandled_exception()
    {
        m_exception = std::current_exception();
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
    std::add_pointer_t<T> m_value;
};

template<>
class promise<void> final : public promise_base<void>
{
public:
    promise() noexcept = default;
    promise(const promise<void>&) = delete;
    promise(promise<void>&&) = delete;

    promise<void>& operator=(const promise<void>&) = delete;
    promise<void>& operator=(promise<void>&&) = delete;

    lazy<void> get_return_object();

    void return_void()
    {
    }

    void unhandled_exception()
    {
        m_exception = std::current_exception();
    }

    void get()
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
class awaitable_base
{
public:
    explicit awaitable_base(std::coroutine_handle<promise<T>> coroutine) noexcept
        : m_coroutine(coroutine)
    {
    }

    awaitable_base(const awaitable_base<T>&) = delete;

    awaitable_base(awaitable_base<T>&& other) noexcept
        : m_coroutine(other.m_coroutine)
    {
        other.m_coroutine = nullptr;
    }

    awaitable_base<T>& operator=(const awaitable_base<T>&) = delete;

    awaitable_base<T>& operator=(awaitable_base<T>&& rhs) noexcept
    {
        m_coroutine = rhs.m_coroutine;
        rhs.m_coroutine = nullptr;
        return *this;
    }

    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept
    {
        get_promise().set_continuation(continuation);
        return m_coroutine;
    }

protected:
    promise<T>& get_promise() noexcept
    {
        return m_coroutine.promise();
    }

private:
    std::coroutine_handle<promise<T>> m_coroutine;
};

template<typename T>
class awaitable final : public awaitable_base<T>
{
public:
    explicit awaitable(std::coroutine_handle<promise<T>> promise) noexcept
        : awaitable_base<T>(promise)
    {
    }

    awaitable(const awaitable<T>&) = delete;
    awaitable(awaitable<T>&& other) noexcept = default;
    awaitable<T>& operator=(const awaitable<T>&) = delete;
    awaitable<T>& operator=(awaitable<T>&& rhs) noexcept = default;

    T await_resume()
    {
        return std::move(this->get_promise().get());
    }
};

template<typename T>
class awaitable<T&> final : public awaitable_base<T&>
{
public:
    explicit awaitable(std::coroutine_handle<promise<T&>> promise) noexcept
        : awaitable_base<T&>(promise)
    {
    }

    awaitable(const awaitable<T&>&) = delete;
    awaitable(awaitable<T&>&& other) noexcept = default;
    awaitable<T&>& operator=(const awaitable<T&>&) = delete;
    awaitable<T&>& operator=(awaitable<T&>&& rhs) noexcept = default;

    T& await_resume()
    {
        return this->get_promise().get();
    }
};

template<>
class awaitable<void> final : public awaitable_base<void>
{
public:
    explicit awaitable(std::coroutine_handle<promise<void>> promise) noexcept
        : awaitable_base(promise)
    {
    }

    awaitable(const awaitable<void>&) = delete;
    awaitable(awaitable<void>&& other) noexcept = default;
    awaitable<void>& operator=(const awaitable<void>&) = delete;
    awaitable<void>& operator=(awaitable<void>&& rhs) noexcept = default;

    void await_resume()
    {
        this->get_promise().get();
    }
};

template<typename T>
class final_awaitable
{
public:
    constexpr bool await_ready() const noexcept
    {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<promise<T>> coroutine) noexcept
    {
        return coroutine.promise().continuation();
    }

    void await_resume() noexcept
    {
    }
};

template<typename T>
class [[nodiscard]] lazy
{
public:
    using promise_type = promise<T>;

    explicit lazy(std::coroutine_handle<promise<T>> coroutine) noexcept
        : m_coroutine(coroutine)
    {
    }

    lazy(const lazy<T>&) = delete;

    lazy(lazy<T>&& other) noexcept
        : m_coroutine(other.m_coroutine)
    {
        other.m_coroutine = nullptr;
    }

    ~lazy()
    {
        if (m_coroutine)
        {
            m_coroutine.destroy();
        }
    }

    lazy<T>& operator=(const lazy<T>&) = delete;
    lazy<T>& operator=(lazy<T>&&) = delete; // As per P1056R1

    awaitable<T> operator co_await() const noexcept
    {
        return awaitable<T>(m_coroutine);
    }

private:
    std::coroutine_handle<promise<T>> m_coroutine;
};

template<typename T>
final_awaitable<T> promise_base<T>::final_suspend() noexcept
{
    return {};
}

template<typename T>
lazy<T> promise<T>::get_return_object()
{
    return lazy<T>(std::coroutine_handle<promise<T>>::from_promise(*this));
}

template<typename T>
lazy<T&> promise<T&>::get_return_object()
{
    return lazy<T&>(std::coroutine_handle<promise<T&>>::from_promise(*this));
}

inline lazy<void> promise<void>::get_return_object()
{
    return lazy<void>(std::coroutine_handle<promise<void>>::from_promise(*this));
}

} // namespace _lazy

template<typename T = void>
using lazy = _lazy::lazy<T>;

} // namespace nonstd