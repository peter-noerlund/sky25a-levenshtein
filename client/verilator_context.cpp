#include "verilator_context.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/post.hpp>
#include <asio/use_awaitable.hpp>
#include <verilated.h>
#include <verilated_vcd_c.h>

namespace tt09_levenshtein
{

VerilatorContext::VerilatorContext(unsigned long int frequency)
    : m_halfPeriod(2000000000UL / frequency)
{
}

VerilatorContext::VerilatorContext(unsigned long int frequency, const std::filesystem::path& vcdFileName)
    : m_halfPeriod(2000000000UL / frequency)
{
    Verilated::traceEverOn(true);

    m_vcd = std::make_unique<VerilatedVcdC>();
    m_top.trace(m_vcd.get(), 99);

    m_vcd->open(vcdFileName.string().c_str());
}

asio::awaitable<void> VerilatorContext::clock()
{
    co_await risingEdge(m_top.clk);
}

asio::awaitable<void> VerilatorContext::clocks(unsigned int count)
{
    for (unsigned int i = 0; i != count; i++)
    {
        co_await clock();
    }
}

asio::awaitable<void> VerilatorContext::init()
{
    auto executor = co_await asio::this_coro::executor;

    m_top.rst_n = 0;
    m_top.ena = 1;

    asio::co_spawn(executor, runClock(), asio::detached);

    co_await clocks(10);

    m_top.rst_n = 1;

    co_await clocks(10);
}

asio::awaitable<void> VerilatorContext::wait(std::chrono::nanoseconds time)
{
    co_await clocks(time.count() / (m_halfPeriod.count() * 2));
}

asio::awaitable<void> VerilatorContext::runClock()
{
    auto executor = co_await asio::this_coro::executor;

    VerilatedContext context;
    context.timeunit(9);
    context.timeprecision(9);

    while (true)
    {
        context.timeInc(m_halfPeriod.count());
        m_time += m_halfPeriod;

        m_top.clk ^= 1;
        m_top.eval();

        if (m_vcd)
        {
            m_vcd->dump(context.time());
        }

        co_await asio::post(executor, asio::use_awaitable);
    }
}

} // namespace tt09_levenshtein
