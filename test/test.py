# SPDX-FileCopyrightText: © 2024 Tiny Tapeout
# SPDX-License-Identifier: Apache-2.0

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles, Edge, FallingEdge, Timer


class Uart(object):
    def __init__(self, dut, period=320, period_units="ns"):
        self._dut = dut
        self._period = period
        self._period_units = period_units
        self._dut.ui_in.value |= 0x08

    async def send(self, value: int) -> None:
        # Start bit
        self._dut.ui_in.value = 0
        await Timer(self._period, units=self._period_units)

        # Data bits
        for i in range(0, 8):
            self._dut.ui_in.value = 0x00 if (value & (1 << i)) == 0 else 0x08
            await Timer(self._period, units=self._period_units)

        # Stop bit
        self._dut.ui_in.value = 0x08
        await Timer(self._period, units=self._period_units)


    async def recv(self, max_clock_cycles=10000) -> int:
        assert self._dut.uo_out[4] == 1

        for i in range(0, max_clock_cycles):
            await ClockCycles(self._dut.clk, 1)
            if self._dut.uo_out[4] == 0:
                break

        assert self._dut.uo_out[4] == 0

        await Timer(self._period * 1.5, units=self._period_units)

        value = 0
        for i in range(0, 8):
            value = value | (1 << i if self._dut.uo_out[4] == 1 else 0)

            await Timer(self._period, units=self._period_units)

        assert self._dut.uo_out[4] == 1

        await Timer(self._period / 2, units=self._period_units)

        return value


class Wishbone(object):
    def __init__(self, transport):
        self._transport = transport

    async def write(self, address: int, data: int) -> None:
        await self._exec(0x80000000 | ((address & 0x7FFFFF) << 8) | data)

    async def read(self, address: int) -> int:
        return await self._exec((address & 0x7FFFFF) << 8)

    async def _exec(self, data: int) -> int:
        await self._transport.send((data >> 24) & 0xFF)
        await self._transport.send((data >> 16) & 0xFF)
        await self._transport.send((data >> 8) & 0xFF)
        await self._transport.send(data & 0xFF)
        return await self._transport.recv()


class Accelerator(object):
    CTRL_ADDR = 0
    DISTANCE_ADDR = 1
    INDEX_ADDR = 2
    VECTORMAP_BASE_ADDR = 0x000200
    DICTIONARY_BASE_ADDR = 0x000400

    ACTIVE_FLAG = 0x80

    def __init__(self, bus):
        self._bus = bus

    async def init(self):
        for i in range(1, 256):
            await self._bus.write(self.VECTORMAP_BASE_ADDR + i * 2, 0)
            await self._bus.write(self.VECTORMAP_BASE_ADDR + i * 2 + 1, 0)

    async def load_dictionary(self, words):
        address = self.DICTIONARY_BASE_ADDR
        for word in words:
            for c in word:
                await self._bus.write(address, ord(c))
                address += 1
            await self._bus.write(address, 0x00)
            address += 1
        await self._bus.write(address, 0x01)

    async def search(self, search_word: str):
        assert (await self._bus.read(self.CTRL_ADDR) & self.ACTIVE_FLAG) == 0

        vector_map = {}
        for c in search_word:
            vector = 0
            for i in range(0, len(search_word)):
                if search_word[i] == c:
                    vector |= (1 << i)
            vector_map[c] = vector

        for c, vector in vector_map.items():
            await self._bus.write(self.VECTORMAP_BASE_ADDR + ord(c) * 2, (vector >> 8) & 0xFF)
            await self._bus.write(self.VECTORMAP_BASE_ADDR + ord(c) * 2 + 1, vector & 0xFF)

        await self._bus.write(self.CTRL_ADDR, len(search_word))

        assert (await self._bus.read(self.CTRL_ADDR) & self.ACTIVE_FLAG) == self.ACTIVE_FLAG

        for i in range(0, 20):
            await Timer(100, units="us")

            ctrl = await self._bus.read(self.CTRL_ADDR)
            if (ctrl & self.ACTIVE_FLAG) == 0:
                break

        assert (ctrl & self.ACTIVE_FLAG) == 0

        for c in vector_map.keys():
            await self._bus.write(self.VECTORMAP_BASE_ADDR + ord(c) * 2, 0x00)
            await self._bus.write(self.VECTORMAP_BASE_ADDR + ord(c) * 2 + 1, 0x00)

        distance = await self._bus.read(self.DISTANCE_ADDR)

        idx_hi = await self._bus.read(self.INDEX_ADDR)
        idx_lo = await self._bus.read(self.INDEX_ADDR + 1)

        idx = (idx_hi << 8) | idx_lo

        return (idx, distance)


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

    uart = Uart(dut)
    wishbone = Wishbone(uart)
    accel = Accelerator(wishbone)

    await accel.init()

    words = ["h", "he", "hes", "hest", "heste", "hesten"]
    await accel.load_dictionary(words)

    result = await accel.search("hest")

    assert result[0] == 3
    assert result[1] == 0

