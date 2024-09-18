# SPDX-FileCopyrightText: © 2024 Tiny Tapeout
# SPDX-License-Identifier: Apache-2.0

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles, Edge, FallingEdge, Timer


async def uart_transmit(dut, value, period=320, period_units="ns"):
    dut.ui_in.value = 0

    await Timer(period, units=period_units)

    for i in range(0, 8):
        dut.ui_in.value = 0x00 if (value & (1 << i)) == 0 else 0x08

        await Timer(period, units=period_units)

    dut.ui_in.value = 0x08

    await Timer(period, units=period_units)


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
        value = value | (1 << i if dut.uo_out[4] == 1 else 0)

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

    CTRL_ADDR = 0x000000
    LENGTH_ADDR = 0x000001
    MASK_ADDR_HI = 0x000002
    MASK_ADDR_LO = 0x000003
    VP_ADDR_HI = 0x000004
    VP_ADDR_LO = 0x000005

    STATUS_ADDR = 0x000000
    DISTANCE_ADDR = 0x000001
    IDX_ADDR_HI = 0x000002
    IDX_ADDR_LO = 0x000003

    DICT_ADDR_BASE = 0x600000
    BITVECTOR_ADDR_BASE = 0x400000

    # Save dictionary

    words = ["h", "he", "hes", "hest", "heste", "hesten"]
    address = DICT_ADDR_BASE
    for word in words:
        for c in word:
            await wb_write(dut, address, ord(c))
            address = address + 1
        await wb_write(dut, address, 0xFE)
        address = address + 1
    await wb_write(dut, address, 0xFF)

    # Clear bitmaps
    for i in range(0, 256 * 2):
        await wb_write(dut, BITVECTOR_ADDR_BASE + i, 0x00)

    search_word = "hest"
    vector_map = dict()
    for c in search_word:
        vector = 0
        for i in range(0, len(search_word)):
            if search_word[i] == c:
                vector = vector | (1 << i)
        vector_map[c] = vector

    for c, vector in vector_map.items():
        await wb_write(dut, BITVECTOR_ADDR_BASE + ord(c) * 2, (vector >> 8) & 0xFF)
        await wb_write(dut, BITVECTOR_ADDR_BASE + ord(c) * 2 + 1, vector & 0xFF)

    await wb_write(dut, LENGTH_ADDR, len(search_word))

    mask = 1 << (len(search_word) - 1)
    await wb_write(dut, MASK_ADDR_HI, mask & 0xFF)
    await wb_write(dut, MASK_ADDR_LO, mask & 0xFF)

    vp = (1 << len(search_word)) - 1
    await wb_write(dut, VP_ADDR_HI, vp)
    await wb_write(dut, VP_ADDR_LO, vp)

    await wb_write(dut, CTRL_ADDR, 0x01)

    for i in range(0, 20):
        await Timer(100, units="us")

        state = await wb_read(dut, STATUS_ADDR)
        if state & 0x01 == 0:
            break

    assert state & 0x01 == 0
    assert state & 0x02 == 0

    distance = await wb_read(dut, DISTANCE_ADDR)

    idx_hi = await wb_read(dut, IDX_ADDR_HI)
    idx_lo = await wb_read(dut, IDX_ADDR_LO)
    
    idx = (idx_hi << 8) + idx_lo

    assert idx == 3
    assert distance == 0