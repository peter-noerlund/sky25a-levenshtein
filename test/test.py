# SPDX-FileCopyrightText: © 2024 Tiny Tapeout
# SPDX-License-Identifier: Apache-2.0

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles, Edge, FallingEdge, Timer


async def spi_sram(dut):
    bit_pos = 0
    byte = 0

    byte_pos = 0
    command = 0
    address = 0
    data = 0

    queue = 0

    memory = dict()

    old_ss_n = int(dut.uio_out[0]) if dut.uio_oe[0] else 0
    old_sck = int(dut.uio_out[3]) if dut.uio_oe[3] else 0
    while True:
        await Edge(dut.clk)

        ss_n = int(dut.uio_out[0]) if dut.uio_oe[0] else 0
        sck = int(dut.uio_out[3]) if dut.uio_oe[3] else 0

        if old_ss_n and not ss_n:
            # Falling edge on SS#
            bit_pos = 0
            byte = 0

            byte_pos = 0
            command = 0
            address = 0
            data = 0

            queue = 0

        if not ss_n and not old_sck and sck:
            # Rising edge on SCK
            byte = ((byte << 1) + (int(dut.uio_out[1]) if dut.uio_oe[1] else 0)) & 0xFF
            bit_pos = bit_pos + 1
            if bit_pos == 8:
                if byte_pos == 0:
                    command = byte
                    assert command == 0x02 or command == 0x03
                elif byte_pos == 1:
                    address = byte << 16
                elif byte_pos == 2:
                    address = address | (byte << 8)
                elif byte_pos == 3:
                    address = address | byte
                    if command == 0x03:
                        queue = memory[address] if address in memory else 0
                else:

                    if command == 0x02:
                        memory[address] = byte

                    address = address + 1

                    if command == 0x03:
                        queue = memory[address] if address in memory else 0
                    else:
                        queue = 0

                byte_pos = byte_pos + 1
                bit_pos = 0
        
        if not ss_n and old_sck and not sck:
            dut.uio_in.value = (int(dut.uio_in) & ~0x04) | (0x04 if queue & 0x80 == 0x80 else 0x00)
            queue = (queue << 1) & 0xFF
        if ss_n:
            dut.uio_in.value = int(dut.uio_in) & ~0x04

        old_ss_n = ss_n
        old_sck = sck


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

    await cocotb.start(spi_sram(dut))

    dut._log.info("Test project behavior")

    await ClockCycles(dut.clk, 10)

    await wb_write(dut, 0x400000, 0xDE)
    await wb_write(dut, 0x400001, 0xAD)
    await wb_write(dut, 0x400002, 0xBE)
    await wb_write(dut, 0x400003, 0xEF)

    assert await wb_read(dut, 0x400000) == 0xDE
    assert await wb_read(dut, 0x400001) == 0xAD
    assert await wb_read(dut, 0x400002) == 0xBE
    assert await wb_read(dut, 0x400003) == 0xEF

    await wb_write(dut, 0x000000, 0x00)
    assert dut.user_project.engine_enabled == 0
    assert dut.user_project.word_length == 0
    assert await wb_read(dut, 0x000000) == 0x00

    await wb_write(dut, 0x000000, 0x04)
    assert dut.user_project.engine_enabled == 0
    assert dut.user_project.word_length == 4
    assert await wb_read(dut, 0x00000000) == 0x04

    await wb_write(dut, 0x000000, 0x9F)
    assert dut.user_project.engine_enabled == 1
    assert dut.user_project.word_length == 31
    assert await wb_read(dut, 0x00000000) == 0x9F

    await wb_write(dut, 0x000000, 0xFF)
    assert dut.user_project.engine_enabled == 1
    assert dut.user_project.word_length == 31
    assert await wb_read(dut, 0x00000000) == 0x9F