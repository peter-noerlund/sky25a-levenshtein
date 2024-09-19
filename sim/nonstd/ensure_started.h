#pragma once

#include "nonstd/lazy.h"

#include <atomic>
#include <coroutine>
#include <exception>
#include <memory>
#include <utility>

namespace nonstd
{

namespace _ensure_started
{

template<typename T>
class task;

template<typename T>
class final_awaitable;

class data_base
{
public:
    constexpr data_base() noexcept = default;

    data_base(const data_base&) = delete;
    data_base(data_base&&) = delete;

    data_base& operator=(const data_base&) = delete;
    data_base& operator=(data_base&&) = delete;

    inline void set_has_value() noexcept
    {
        m_state.fetch_add(value_bit, std::memory_order_acq_rel);
    }

    inline void set_has_exception() noexcept
    {
        m_state.fetch_add(exception_bit, std::memory_order_acq_rel);
    }

    inline std::coroutine_handle<> set_finished()
    {
        if (m_state.fetch_add(finished_bit, std::memory_order_acq_rel) & continuation_bit)
        {
            return m_continuation;
        }
        else
        {
            return std::noop_coroutine();
        }
    }

    inline std::coroutine_handle<> set_continuation(std::coroutine_handle<> continuation)
    {
        m_continuation = continuation;
        if (m_state.fetch_add(continuation_bit, std::memory_order_acq_rel) & finished_bit)
        {
            return m_continuation;
        }
        else
        {
            return std::noop_coroutine();
        }
    }
    
    bool has_exception() const noexcept
    {
        return m_state.load(std::memory_order_relaxed) & exception_bit;
    }

    bool has_value() const noexcept
    {
        return m_state.load(std::memory_order_relaxed) & value_bit;
    }

    bool has_finished() const noexcept
    {
        return m_state.load(std::memory_order_relaxed) & finished_bit;
    }

private:
    enum State
    {
        value_bit = 0x01,
        exception_bit = 0x02,
        finished_bit = 0x04,
        continuation_bit = 0x08
    };

    std::atomic_int m_state = 0;
    std::coroutine_handle<> m_continuation;
};

template<typename T>
class data : public data_base
{
public:
    data() noexcept
    {
    }

    ~data()
    {
        if (has_exception())
        {
            m_exception.~exception_ptr();
        }
        else if (has_value())
        {
            m_value.~T();
        }
    }

    template<typename U>
    void set_value(U&& value)
    {
        new(&m_value) T(std::forward<U>(value));
        set_has_value();
    }

    void set_exception(std::exception_ptr exception)
    {
        new(&m_exception) std::exception_ptr(std::move(exception));
        set_has_exception();
    }

    T get()
    {
        if (has_exception())
        {
            std::rethrow_exception(m_exception);
        }
        else
        {
            return m_value;
        }
    }
    
private:
    union
    {
        T m_value;
        std::exception_ptr m_exception;
    };  
};

template<typename T>
class data<T&> : public data_base
{
public:
    data() noexcept
    {

    }

    ~data()
    {
        if (has_exception())
        {
            m_exception.~exception_ptr();
        }
    }

    void set_value(T& value)
    {
        new(&m_value) T*(std::addressof(value));
        set_has_value();
    }

    void set_exception(std::exception_ptr exception)
    {
        new(&m_exception) std::exception_ptr(exception);
        set_has_exception();
    }

    T& get()
    {
        if (has_exception())
        {
            std::rethrow_exception(m_exception);
        }
        else
        {
            return *m_value;
        }
    }

private:
    union
    {
        T* m_value;
        std::exception_ptr m_exception;
    };
};

template<>
class data<void> : public data_base
{
public:
    data() noexcept = default;

    inline void set_value()
    {
        set_has_value();
    }

    inline void set_exception(std::exception_ptr exception)
    {
        m_exception = exception;
        set_has_exception();
    }

    inline void get()
    {
        if (has_exception())
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
    promise_base()
        : m_data(std::make_shared<data<T>>())
    {
    }

    promise_base(const promise_base<T>&) = delete;
    promise_base(promise_base<T>&&) = delete;

    promise_base<T>& operator=(const promise_base<T>&) = delete;
    promise_base<T>& operator=(promise_base<T>&&) = delete;

    task<T> get_return_object();

    std::suspend_never initial_suspend() noexcept
    {
        return {};
    }

    final_awaitable<T> final_suspend() noexcept;

    void unhandled_exception() noexcept
    {
        m_data->set_exception(std::current_exception());
    }

    std::coroutine_handle<> set_finished()
    {
        return m_data->set_finished();
    }

protected:
    std::shared_ptr<data<T>> m_data;
};

template<typename T>
class promise final : public promise_base<T>
{
public:
    void return_value(const T& value)
    {
        this->m_data->set_value(value);
    }

    void return_value(T&& value)
    {
        this->m_data->set_value(std::move(value));
    }
};

template<typename T>
class promise<T&> final : public promise_base<T&>
{
public:
    void return_value(T& value) noexcept
    {
        this->m_data->set_value(value);
    }
};

template<>
class promise<void> final : public promise_base<void>
{
public:
    inline void return_void() noexcept
    {
        this->m_data->set_value();
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
        return coroutine.promise().set_finished();
    }

    void await_resume() noexcept
    {
    }
};

template<typename T>
class awaitable
{
public:
    explicit awaitable(std::shared_ptr<data<T>> data) noexcept
        : m_data(std::move(data))
    {
    }

    awaitable(const awaitable<T>&) = delete;

    awaitable(awaitable<T>&& other) noexcept
        : m_data(std::move(other.m_data))
    {
    }

    awaitable<T>& operator=(const awaitable<T>&) = delete;

    awaitable<T>& operator=(awaitable<T>&& rhs) noexcept
    {
        if (&rhs != this)
        {
            m_data = std::move(rhs.m_data);
        }
        return *this;
    }

    bool await_ready() const noexcept
    {
        return m_data->has_finished();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept
    {
        return m_data->set_continuation(continuation);
    }

    T await_resume()
    {
        if constexpr (std::is_same_v<T, void>)
        {
            m_data->get();
        }
        else
        {
            return m_data->get();
        }
    }

private:
    std::shared_ptr<data<T>> m_data;
};

template<typename T>
class [[nodiscard]] task
{
public:
    using promise_type = promise<T>;

    explicit task(std::shared_ptr<data<T>> data) noexcept
        : m_data(std::move(data))
    {
    }

    task(const task<T>&) = delete;

    task(task<T>&& other) noexcept
        : m_data(std::move(other.m_data))
    {
    }

    task<T>& operator=(const task<T>&) = delete;
    task<T>& operator=(task<T>&&) = delete;

    awaitable<T> operator co_await() const noexcept
    {
        return awaitable<T>(m_data);
    }

private:
    std::shared_ptr<data<T>> m_data;
};

template<typename T>
task<T> promise_base<T>::get_return_object()
{
    return task<T>(m_data);
}

template<typename T>
final_awaitable<T> promise_base<T>::final_suspend() noexcept
{
    return {};
}

struct adapter
{
};

template<typename T>
task<T> start(lazy<T> task)
{
    co_return co_await task;
}

template<typename T>
lazy<T> to_lazy(task<T> task)
{
    co_return co_await task;
}

} // namespace _ensure_started

template<typename T>
nonstd::lazy<T> ensure_started(lazy<T> task)
{
    auto started_task = _ensure_started::start(std::move(task));
    return to_lazy(std::move(started_task));
}

constexpr _ensure_started::adapter ensure_started() noexcept
{
    return {};
}

} // namespace nonstd

template<typename T>
nonstd::lazy<T> operator|(nonstd::lazy<T> task, nonstd::_ensure_started::adapter)
{
    return nonstd::ensure_started(std::move(task));
}