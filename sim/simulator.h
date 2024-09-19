#pragma once

#include "nonstd/lazy.h"
#include "nonstd/make_awaitable.h"
#include "nonstd/move_only_function.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

template<typename Vtop>
class Simulator
{
public:
    Simulator() = default;

    Simulator(const std::filesystem::path& vcdFileName)
        : m_vcdFileName(vcdFileName)
    {

    }

    constexpr Vtop& top() noexcept
    {
        return m_top;
    }

    constexpr const Vtop& top() const noexcept
    {
        return m_top;
    }

    void run()
    {
        VerilatedContext context;
        context.timeunit(8);
        context.timeprecision(8);
        
        m_running = true;
        while (m_running)
        {
            context.timeInc(1);
            
            m_top.clk ^= 1;
            m_top.eval();
            auto callbacks = std::move(m_callbacks);
            for (auto& callback : callbacks)
            {
                if (!m_running)
                {
                    break;
                }
                callback(*this);
            }
        }
    }

    void stop()
    {
        m_running = false;
    }

    template<typename F>
    void onNextEvent(F&& callback)
    {
        m_callbacks.emplace_back(std::forward<F>(callback));
    }

    nonstd::lazy<void> nextEvent()
    {
        co_return co_await nonstd::make_awaitable<void>([this](auto&& promise)
        {
            m_callbacks.emplace_back([promise = std::move(promise)](auto&) mutable
            {
                promise.set_value();
            });
        });
    }

    template<typename T, typename V = int>
    nonstd::lazy<void> risingEdge(T& pin, V mask = 1)
    {
        auto oldValue = pin & mask;
        while (true)
        {
            co_await nextEvent();
            auto newValue = pin & mask;
            if (!oldValue && newValue)
            {
                break;
            }
            oldValue = newValue;
        }
    }

    template<typename T, typename V = int>
    nonstd::lazy<void> fallingEdge(T& pin, V mask = 1)
    {
        auto oldValue = pin & mask;
        while (true)
        {
            co_await nextEvent();
            auto newValue = pin & mask;
            if (oldValue && !newValue)
            {
                break;
            }
            oldValue = newValue;
        }
    }

    template<typename T, typename V = int>
    nonstd::lazy<void> edge(T& pin, V mask = 1)
    {
        auto oldValue = pin & mask;
        while (true)
        {
            co_await nextEvent();
            auto newValue = pin & mask;
            if (oldValue != newValue)
            {
                break;
            }
            oldValue = newValue;
        }
    }

    nonstd::lazy<void> clock()
    {
        co_return co_await risingEdge(m_top.clk);
    }

    nonstd::lazy<void> clocks(unsigned int count)
    {
        for (unsigned int i = 0; i != count; ++i)
        {
            co_await clock();
        }
    }

private:
    Vtop m_top;
    bool m_running = false;
    std::vector<nonstd::move_only_function<void(Simulator&)>> m_callbacks;
    std::filesystem::path m_vcdFileName;
};