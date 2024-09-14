# SPDX-FileCopyrightText: © 2024 Tiny Tapeout
# SPDX-License-Identifier: Apache-2.0

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles, FallingEdge, Timer


async def uart_transmit(dut, value):
    dut.ui_in.value = 0

    await Timer(320, units="ns")

    for i in range(0, 8):
        dut.ui_in.value = 0x00 if (value & (1 << i)) == 0 else 0x08

        await Timer(320, units="ns")

    dut.ui_in.value = 0x08

    await Timer(320, units="ns")


async def uart_receive(dut, max_clock_cycles=10000, period=320, period_units="ns"):
    value = 0

    assert dut.uo_out[4] == 1

    for i in range(0, max_clock_cycles):
        await ClockCycles(dut.clk, 1)
        if dut.uo_out[4] == 0:
            break

    assert dut.uo_out[4] == 0

    await Timer(period * 1.5, units=period_units)

    for i in range(0, 8):
        value = (value << 1) | (1 if dut.uo_out[4] == 1 else 0)

        await Timer(period, units=period_units)

    assert dut.uo_out[4] == 1

    await Timer(period / 2, units=period_units)

    return value


async def wb_write(dut, addr, data):
    await uart_transmit(dut, 0x80 | ((addr >> 16) & 0x7F))
    await uart_transmit(dut, (addr >> 8) & 0xFF)
    await uart_transmit(dut, addr & 0xFF)
    await uart_transmit(dut, data)
    await uart_receive(dut)


async def wb_read(dut, addr):
    await uart_transmit(dut, (addr >> 16) & 0x7F)
    await uart_transmit(dut, (addr >> 8) & 0xFF)
    await uart_transmit(dut, addr & 0xFF)
    await uart_transmit(dut, 0)
    return await uart_receive(dut)


@cocotb.test()
async def test_project(dut):
    dut._log.info("Start")

    clock = Clock(dut.clk, 20, units="ns")
    cocotb.start_soon(clock.start())

    # Reset
    dut._log.info("Reset")
    dut.ena.value = 1
    dut.ui_in.value = 0x08
    dut.uio_in.value = 0
    dut.rst_n.value = 0
    await ClockCycles(dut.clk, 10)
    dut.rst_n.value = 1

    dut._log.info("Test project behavior")

    await ClockCycles(dut.clk, 10)

    await wb_write(dut, 0, 0xDE)
    await wb_write(dut, 1, 0xAD)
    await wb_write(dut, 2, 0xBE)
    await wb_write(dut, 3, 0xEF)

    await wb_read(dut, 0)
    await wb_read(dut, 1)
    await wb_read(dut, 2)
    await wb_read(dut, 3)
