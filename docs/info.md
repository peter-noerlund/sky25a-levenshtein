<!---

This file is used to generate your project datasheet. Please fill in the information below and delete any unused
sections.

You can also include images in this folder and reference them in the markdown. Each image must be less than
512 kb in size, and the combined size of all images must be less than 1 MB.
-->

## How it works

tt09-levenshtein is a fuzzy search engine which can find the best matching word in a dictionary based on levenshtein distance.

### Memory layout


| Address  | Size | Description    |
|----------|------|----------------|
| 0x000000 |   6B | Control/Status |
| 0x400000 | 128B | Bitvectors     |
| 0x600000 |  2MB | Dictionary     |


## How to test

Explain how to use your project

## External hardware

List external hardware used in your project (e.g. PMOD, LED display, etc), if any
