<!---

This file is used to generate your project datasheet. Please fill in the information below and delete any unused
sections.

You can also include images in this folder and reference them in the markdown. Each image must be less than
512 kb in size, and the combined size of all images must be less than 1 MB.
-->

## How it works

tt09-levenshtein is a fuzzy search engine which can find the best matching word in a dictionary based on levenshtein distance.

Fundamentally its an implementation of the bit-vector levenshtein algorithm from Heikki Hyyrö's 2022 paper with the title *A Bit-Vector Algorithm for Computing Levenshtein and Damerau Edit Distances*.

#### UART

The device is organized as a wishbone bus which is accessed through commands on the UART.

Each command consists of 4 input bytes and 1 output byte:

**Input bytes:**

| Byte | Bit | Description                               |
|------|-----|-------------------------------------------|
| 0    | 7   | READ=`0` WRITE=`1`                        |
| 0    | 6-0 | Address bit 22-16                         |
| 1    | 7-0 | Address bit 15-8                          |
| 2    | 7-0 | Address bit 7-0                           |
| 3    | 7-0 | Byte to write if WRITE, otherwise ignored |

**Output bytes:**

| Byte | Bit | Description                              |
|------|-----|------------------------------------------|
| 0    | 7-0 | Byte read if READ, otherwise just `0x00` |


#### Memory Layout

As indicated by the UART protocol, the address space is 23 bits.

The lower half of the memory space is used for registers and the upper half of the memory space is accessing an external SPI PSRAM.

The address space is basically as follows:

| Address  | Size | Access | Identifier  | Type | Description                                          |
|----------|------|--------|-------------|------|------------------------------------------------------|
| 0x000000 | 1    | R/W    | `CTRL`      | Reg  | Control register. Bit 0-4=Word length. Bit 7=Running |
| 0x000001 | 1    | R/O    | `DISTANCE`  | Reg  | Levenshtein distance                                 |
| 0x000002 | 2    | R/O    | `INDEX`     | Reg  | Word index                                           |
| 0x400000 | 512  | R/W    | `VECTORMAP` | SPI  | Vector map                                           |
| 0x600000 | 2M   | R/W    | `DICT`      | SPI  | Dictionary                                           |

#### Operation

##### Initialization

Before doing anything, the vector map (`VECTORMAP`) needs to be filled with `0x00`. This is to reduce the work necessary when starting a search and is only necessary once after power up.

##### Store dictionary

Next, you need to store a dictionary in the dictionary memory (`DICT`). The dictionary is stored as a sequence of words, encoded using 1 bit per character. Each word is terminated with the byte value `0xFE` and the dictionary itself is terminated by the byte value `0xFF`. In total there can be no more than 65535 words and the whole list must not exceed 2MB.

##### Perform fuzzy matching

To perform a fuzzy search, you first need to populate the vector map (`VECTORMAP`) based on the input word.

For each character in the word, you produce a bit vector representing which position in the word holds the character.

Example:

```verilog
word = application

a = 16'b00000000_01000001;  // a_____a____
p = 16'b00000000_00000110;  // _pp________
l = 16'b00000000_00001000;  // ___l_______
i = 16'b00000001_00010000;  // ____i___i__
c = 16'b00000000_00100000;  // _____c_____
t = 16'b00000000_10000000;  // _______t___
o = 16'b00000010_00000000;  // _________o_
n = 16'b00000100_00000000;  // __________n
```

You then store each bitvector at address `VECTORMAP + char * 2`. The bitvectors is stored in bit endian byte order.

Finally, you store the length in the `CTRL`register which will then initiate the accelerator

To know when the algorithm is done, you poll the control register (`CTRL`) at a regular interval until the most significant bit is 0.

You can then read out the levenshtein distance at address `DISTANCE` and the index of the word in the dictionary which was the best match at `INDEX` (big endian).

Finally, you need to clear the vector map (`VECTORMAP`) before the next search. Instead of filling the entire 512 bytes with `0x00`, you simply clear the bitvector positions you set earlier (in the example that would be `a`, `p`, `l`, `i`, `c`, `t`, `o`, and `n`)

## How to test

TODO

## External hardware

List external hardware used in your project (e.g. PMOD, LED display, etc), if any
