# SPDX-FileCopyrightText: © 2024 Tiny Tapeout
# SPDX-License-Identifier: Apache-2.0

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles
from cocotb.triggers import Timer
from cocotb.triggers import FallingEdge

async def uart_transmit(dut, value):
    dut.ui_in.value = 0

    await Timer(320, units="ns")

    for i in range(0, 8):
        dut.ui_in.value = 0x00 if (value & (1 << i)) == 0 else 0x08

        await Timer(320, units="ns")

    dut.ui_in.value = 0x08

    await Timer(320, units="ns")


async def uart_receive(dut):
    value = 0

    await FallingEdge(dut.uo_out[4])

    await Timer(480, units="ns")

    for i in range(0, 8):
        value = (value << 1) | (1 if dut.uo_out[4] else 0)

        await Timer(320, units="ns")

    assert dut.uo_out[4]

    await Timer(120, units="ns")

    return value
            


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

    await uart_transmit(dut, 0x80)
    await uart_transmit(dut, 0x00)
    await uart_transmit(dut, 0x00)
    await uart_transmit(dut, 0xFF)
    await Timer(10, units="us")

    await uart_transmit(dut, 0x00)
    await uart_transmit(dut, 0x00)
    await uart_transmit(dut, 0x00)
    await uart_transmit(dut, 0x00)
    await Timer(10, units="us")
