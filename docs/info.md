<!---

This file is used to generate your project datasheet. Please fill in the information below and delete any unused
sections.

You can also include images in this folder and reference them in the markdown. Each image must be less than
512 kb in size, and the combined size of all images must be less than 1 MB.
-->

## How it works

tt09-levenshtein is a fuzzy search engine which can find the best matching word in a dictionary based on levenshtein distance.

Fundamentally its an implementation of the bit-vector levenshtein algorithm from Heikki Hyyrö's 2022 paper with the title *A Bit-Vector Algorithm for Computing Levenshtein and Damerau Edit Distances*.

### UART

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


### Memory Layout

As indicated by the UART protocol, the address space is 23 bits.

The lower half of the memory space is used for registers and the upper half of the memory space is accessing an external SPI PSRAM.

The address space is basically as follows:

| Address  | Size | Access | Identifier  |
|----------|------|--------|-------------|
| 0x000000 | 1    | R/W    | `CTRL`      |
| 0x000001 | 1    | R/O    | `DISTANCE`  |
| 0x000002 | 2    | R/O    | `INDEX`     |
| 0x400000 | 512  | R/W    | `VECTORMAP` |
| 0x400200 | 4M   | R/W    | `DICT`      |

**CTRL**

The control register is used to start the engine and see when it has completed.

The layout is as follows:

| Bits | Size | Access | Description                                                 |
|------|------|--------|-------------------------------------------------------------|
| 0-4  | 4    | R/W    | Word length                                                 |
| 5-6  | 2    | R/O    | Not used                                                    |
| 7    | 1    | R/O    | Is set to `1` while the engine runs and `0` when it is done |

When data is written to this address, the engine automatically starts.

**DISTANCE**

When the engine has finished executing, this address contains the levenshtein distance of the best match.

**INDEX**

When the engine has finished executing, this address contains the index of the best word from the dictionary.

**VECTORMAP**

The vector map must contain the corresponding bitvector for each input byte in the alphabet.

If the search word is `application`, the bit vectors will look as follows:

| Letter | Index  | Bit vector                              |
|--------|--------|-----------------------------------------|
| `a`    | `0x61` | `16'b00000000_01000001` (`a_____a____`) |
| `p`    | `0x70` | `16'b00000000_00000110` (`_pp________`) |
| `l`    | `0x6C` | `16'b00000000_00001000` (`___l_______`) |
| `i`    | `0x69` | `16'b00000001_00010000` (`____i___i__`) |
| `c`    | `0x63` | `16'b00000000_00100000` (`_____c_____`) |
| `t`    | `0x74` | `16'b00000000_10000000` (`_______t___`) |
| `o`    | `0x6F` | `16'b00000010_00000000` (`_________o_`) |
| `n`    | `0x6E` | `16'b00000100_00000000` (`__________n`) |
| *      | *      | `16'b00000000_00000000` (`___________`) |

Since each vector is 16-bit, the corresponding address is `0x400000 + index * 2`

**DICT**

The word list.

The word list is stored of a sequence of words, each encoded as a sequence of 8-bit characters and terminated by the byte value `0x00`. The list itself is terminated with the byte value `0x01`.

Note that the algorithm doesn't care about the particular characters. It only cares if they are identical or not, so even though the algorithm doesn't support UTF-8 and is limited to a character set of 254 characters,
ignoring Asian alphabets, a list of words usually don't contain more than 254 distinct characters, so you can practially just map lettters to a value between 2 and 255.

### Operation

**Initialization**



## How to test

You can compile the client as follows:

```sh
mkdir -p build
cmake -G Ninja -B build .
cmake --build build
```

Next, you can run the test tool:

```sh
./build/client/client --device=/dev/ttyUSB2 --test
```

This will load 1024 words of random length and characters into the SRAM and then perform a bunch of searches, verifying that the returned result is correct.

## External hardware

To operate, the device needs a SPI PSRAM PMOD. The design is tested with QQSPI PSRAM PMOD from Machdyne, but any memory PMOD will work as long as it supports the zero-latency READ (`0x03`) and WRITE (`0x02`) commands as well as using pin 0 as `SS#`. Note, that this makes the SRAM/Flash PMOD from mole99 incompatible.

The spi-ram-emu project for the RP2040 can be used to emulate a SRAM PMOD, but it would need to be modified to use 24-bit addresses to work.