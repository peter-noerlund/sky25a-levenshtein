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


class SPIWishbone(object):
    SS_N = 0x10
    SCK = 0x20
    MOSI = 0x40
    MISO = 0x80
    UI_IN_MASK = ~0x70

    def __init__(self, dut, period=50, period_units="ns"):
        self._dut = dut
        self._half_period = period / 2
        self._half_period_units = period_units
        self._dut.ui_in.value = (int(self._dut.ui_in) & ~0x60) | 0x10   # Deassert SS# and clear MOSI and SCK

    async def write(self, address: int, data: int) -> None:
        await self._exec(0x80000000 | ((address & 0x7FFFFF) << 8) | data)
    
    async def read(self, address: int) -> int:
        return await self._exec((address & 0x7FFFFF) << 8)

    async def _exec(self, data: int) -> int:
        # Transmit bits
        for i in range(0, 32):
            if data & 0x80000000 == 0:
                self._dut.ui_in.value = self._dut.ui_in.value & self.UI_IN_MASK
            else:
                self._dut.ui_in.value = (self._dut.ui_in.value & self.UI_IN_MASK) | self.MOSI
            await Timer(self._half_period, units=self._half_period_units)

            if data & 0x80000000 == 0:
                self._dut.ui_in.value = (self._dut.ui_in.value & self.UI_IN_MASK) | self.SCK
            else:
                self._dut.ui_in.value = (self._dut.ui_in.value & self.UI_IN_MASK) | (self.MOSI | self.SCK)
            await Timer(self._half_period, units=self._half_period_units)

            data <<= 1

        # Wait for response
        for i in range(0, 1000):
            self._dut.ui_in.value = self._dut.ui_in.value & self.UI_IN_MASK
            await Timer(self._half_period, units=self._half_period_units)

            if self._dut.uo_out[7] == 1:
                break

            self._dut.ui_in.value = (self._dut.ui_in.value & self.UI_IN_MASK) | self.SCK
            await Timer(self._half_period, units=self._half_period_units)

        assert self._dut.uo_out[7] == 1

        # Read response byte
        value = 0
        for i in range(0, 8):
            self._dut.ui_in.value = (self._dut.ui_in.value & self.UI_IN_MASK) | self.SCK
            await Timer(self._half_period, units=self._half_period_units)

            self._dut.ui_in.value = self._dut.ui_in.value & self.UI_IN_MASK
            await Timer(self._half_period, units=self._half_period_units)

            value = (value << 1) | (1 if self._dut.uo_out[7] == 1 else 0)

        self._dut.ui_in.value = (self._dut.ui_in.value & self.UI_IN_MASK) | self.SS_N
        await Timer(self._half_period, units=self._half_period_units)

        return value


class UARTWishbone(object):
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
    SRAM_CTRL_ADDR = 1
    LENGTH_ADDR = 2
    MAX_LENGTH_ADDR = 3
    INDEX_ADDR = 4
    DISTANCE_ADDR = 6

    ENABLE_FLAG = 1

    def __init__(self, bus):
        self._bus = bus

    async def init(self, sram_select: int):
        self._max_length = await self._bus.read(self.MAX_LENGTH_ADDR) + 1
        self._bitvector_size = ((self._max_length + 7) // 8) * 8
        if self._bitvector_size > 128:
            self._bitvector_alignment = 32
        elif self._bitvector_size > 64:
            self._bitvector_alignment = 16
        elif self._bitvector_size > 32:
            self._bitvector_alignment = 8
        elif self._bitvector_size > 16:
            self._bitvector_alignment = 4
        elif self._bitvector_size > 8:
            self._bitvector_alignment = 2
        else:
            self._bitvector_alignment = 1

        vectormap_size = 256 * self._bitvector_alignment
        self._vectormap_base_addr = vectormap_size
        self._dictionary_base_addr = vectormap_size * 2

        await self._bus.write(self.SRAM_CTRL_ADDR, sram_select)
        for i in range(0, 256):
            for j in range(0, self._bitvector_size // 8):
                await self._bus.write(self._vectormap_base_addr + i * self._bitvector_alignment + j, 0)

    async def load_dictionary(self, words):
        assert (await self._bus.read(self.CTRL_ADDR) & self.ENABLE_FLAG) == 0

        address = self._dictionary_base_addr
        for word in words:
            for c in word:
                await self._bus.write(address, ord(c))
                address += 1
            await self._bus.write(address, 0x00)
            address += 1
        await self._bus.write(address, 0x01)

    async def verify_dictionary(self, words) -> bool:
        assert (await self._bus.read(self.CTRL_ADDR) & self.ENABLE_FLAG) == 0

        address = self._dictionary_base_addr
        for word in words:
            for c in word:
                b = await self._bus.read(address)
                if b != ord(c):
                    return False
                address += 1

            b = await self._bus.read(address)
            if b != 0:
                return False

            address += 1

        b = await self._bus.read(address)
        return b == 0x01

    async def search(self, search_word: str):
        assert (await self._bus.read(self.CTRL_ADDR) & self.ENABLE_FLAG) == 0
        assert len(search_word) > 0
        assert len(search_word) <= self._max_length

        vector_map = {}
        for c in search_word:
            vector = 0
            for i in range(0, len(search_word)):
                if search_word[i] == c:
                    vector |= (1 << i)
            vector_map[c] = vector

        for c, vector in vector_map.items():
            for i in range(0, self._bitvector_size // 8):
                val = (vector >> (self._bitvector_size - 8 - i * 8)) & 0xFF
                if val != 0:
                    await self._bus.write(self._vectormap_base_addr + ord(c) * self._bitvector_alignment + i, val)

        await self._bus.write(self.LENGTH_ADDR, len(search_word) - 1)
        await self._bus.write(self.CTRL_ADDR, self.ENABLE_FLAG)

        assert (await self._bus.read(self.CTRL_ADDR) & self.ENABLE_FLAG) == self.ENABLE_FLAG

        for i in range(0, 20):
            await Timer(100, units="us")

            ctrl = await self._bus.read(self.CTRL_ADDR)
            if (ctrl & self.ENABLE_FLAG) == 0:
                break

        assert (ctrl & self.ENABLE_FLAG) == 0

        for c in vector_map.keys():
            for i in range(0, self._bitvector_size // 8):
                val = (vector >> (self._bitvector_size - 8 - i * 8)) & 0xFF
                if val != 0:
                    await self._bus.write(self._vectormap_base_addr + ord(c) * self._bitvector_alignment + i, 0x00)

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
    dut.ui_in.value = 0x18
    dut.uio_in.value = 0
    dut.rst_n.value = 0
    await ClockCycles(dut.clk, 10)
    dut.rst_n.value = 1

    dut._log.info("Test project behavior")

    await ClockCycles(dut.clk, 10)

    #uart = Uart(dut)
    wishbone = SPIWishbone(dut, period=80, period_units="ns")
    accel = Accelerator(wishbone)


    dictionary = ["h", "he", "hes", "hest", "heste", "hesten"]

    await accel.init(1)
    await accel.load_dictionary(dictionary)
    assert await accel.verify_dictionary(dictionary)

    result = await accel.search("hest")

    assert result[0] == 3
    assert result[1] == 0

    await accel.init(2)
    await accel.load_dictionary(dictionary)
    assert not await accel.verify_dictionary(dictionary)

    await accel.init(3)
    await accel.load_dictionary(dictionary)
    assert not await accel.verify_dictionary(dictionary)



