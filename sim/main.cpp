#include "accelerator.h"
#include "nonstd/ensure_started.h"
#include "nonstd/lazy.h"
#include "uart.h"
#include "wishbone.h"
#include "Vtop.h"

#include <fmt/printf.h>
#include <verilated.h>

#include <array>
#include <string_view>

template<typename Vtop>
nonstd::lazy<void> runTest(Simulator<Vtop>& sim)
{
    using UartType = Uart<Vtop>;
    using WishboneType = Wishbone<Vtop, UartType>;
    using AcceleratorType = Accelerator<Vtop, WishboneType>;

    UartType uart(sim);
    WishboneType bus(sim, uart);
    AcceleratorType accel(sim, bus);

    co_await accel.init();

    auto dictionary = std::to_array<std::string_view>({"h", "he", "hes", "hest", "heste", "hesten"});
    co_await accel.loadDictionary(dictionary);

    auto result = co_await accel.search("hest");
    fmt::println("idx={}  distance={}", result.index, result.distance);

    sim.stop();
}

template<typename Vtop>
nonstd::lazy<void> stopAtClock(Simulator<Vtop>& sim, unsigned int maxClocks)
{
    co_await sim.clocks(maxClocks);
    fmt::println("Timeout");
    sim.stop();
}

int main(int argc, char** argv)
{
	Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    Simulator<Vtop> simulator;

    auto mainTest = runTest(simulator) | nonstd::ensure_started();
    auto abortTask = stopAtClock(simulator, 1024 * 1024 * 1024) | nonstd::ensure_started();

    simulator.run();

    return EXIT_SUCCESS;
}