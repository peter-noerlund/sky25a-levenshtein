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
                        print(f"READ(0x{address:06x}) = 0x{queue:02x}")
                else:

                    if command == 0x02:
                        print(f"WRITE(0x{address:06x}, 0x{byte:02x})")
                        memory[address] = byte

                    address = address + 1

                    if command == 0x03:
                        #print(f"READ(0x{address:06x}) = 0x{queue:02x}")
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

    CTRL_ADDR = 0x000000
    MASK_ADDR = 0x000001
    VP_ADDR = 0x000002
    DICT_ADDR_BASE = 0x600000
    BITVECTOR_ADDR_BASE = 0x400000
    RESULT_ADDR_BASE = 0x500000

    words = ["hest", "bil"]
    address = DICT_ADDR_BASE
    for word in words:
        await wb_write(dut, address, 0x80 | len(word))
        address = address + 1
        for c in word:
            await wb_write(dut, address, ord(c))
            address = address + 1
    await wb_write(dut, address, 0xFF)

    # word = fest
    await wb_write(dut, BITVECTOR_ADDR_BASE + ord("f"), 0x01)
    await wb_write(dut, BITVECTOR_ADDR_BASE + ord("e"), 0x02)
    await wb_write(dut, BITVECTOR_ADDR_BASE + ord("s"), 0x04)
    await wb_write(dut, BITVECTOR_ADDR_BASE + ord("t"), 0x08)

    await wb_write(dut, MASK_ADDR, 0x0F)
    await wb_write(dut, VP_ADDR, 0x07)
    await wb_write(dut, CTRL_ADDR, 0x84)

    for i in range(0, 2):
        await Timer(10, units="us")

        state = await wb_read(dut, 0x00000000)
        if state & 0x80 == 0:
            break

    assert state & 0x80 == 0
    assert state & 0x40 == 0

    results = []
    results = results + [await wb_read(dut, 0x500001)]
    results = results + [await wb_read(dut, 0x500002)]

    print(results)