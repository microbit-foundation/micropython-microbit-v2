#!/usr/bin/env python3

"""
Add a flash layout table to a hex firmware for MicroPython on the micro:bit.

Usage: ./addlayouttable.py <firmware.hex> <firmware.map> [-o <combined.hex>]

Output goes to stdout if no filename is given.

The layout table is a sequence of 16-byte entries.  The last entry contains the
header (including magic numbers) and is aligned to the end of a page such that
the final byte of the layout table is the final byte of the page it resides in.
This is so it can be quickly and easily searched for.

The layout table has the following format.  All integer values are unsigned and
store little endian.

0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f

ID    HT    REG_PAGE    REG_LEN                 HASH_DATA
(additional regions)
...
MAGIC1                  VERSION     TABLE_LEN   NUM_REG     PSIZE_LOG2  MAGIC2

The values are:

ID         - 1 byte  - region id for this entry, defined by the region
HT         - 1 byte  - hash type of the region hash data
REG_PAGE   - 2 bytes - starting page number of the region
REG_LEN    - 4 bytes - length in bytes of the region
HASH_DATA  - 8 bytes - data for the hash of this region
                       HT=0: hash data is empty
                       HT=1: hash data contains 8 bytes of verbatim data
                       HT=2: hash data contains a 4-byte pointer to a string

MAGIC1     - 4 bytes - 0x597F30FE
VERSION    - 2 bytes - table version (currently 1)
TABLE_LEN  - 2 bytes - length in bytes of the table excluding this header row
NUM_REG    - 2 bytes - number of regions
PSIZE_LOG2 - 2 bytes - native page size of the flash, log-2
MAGIC2     - 4 bytes - 0xC1B1D79D

"""

import argparse
import binascii
import struct
import sys

IHEX_TYPE_DATA = 0
IHEX_TYPE_EXT_LIN_ADDR = 4

NRF_PAGE_SIZE_LOG2 = 12
NRF_PAGE_SIZE = 1 << NRF_PAGE_SIZE_LOG2


class FlashLayout:
    MAGIC1 = 0x597F30FE
    MAGIC2 = 0xC1B1D79D
    VERSION = 1

    REGION_HASH_NONE = 0
    REGION_HASH_DATA = 1
    REGION_HASH_PTR = 2

    def __init__(self):
        self.data = b""
        self.num_regions = 0

    def add_region(
        self, region_id, region_addr, region_len, region_hash_type, region_hash=None
    ):
        # Compute/validate the hash data.
        if region_addr % NRF_PAGE_SIZE != 0:
            assert 0, region_addr
        if region_hash_type == FlashLayout.REGION_HASH_NONE:
            assert region_hash is None
            region_hash = b"\x00" * 8
        elif region_hash_type == FlashLayout.REGION_HASH_DATA:
            assert len(region_hash) == 8
        elif region_hash_type == FlashLayout.REGION_HASH_PTR:
            region_hash = struct.pack("<II", region_hash, 0)

        # Increase number of regions.
        self.num_regions += 1

        # Add the region data.
        self.data += struct.pack(
            "<BBHI8s",
            region_id,
            region_hash_type,
            region_addr // NRF_PAGE_SIZE,
            region_len,
            region_hash,
        )

    def finalise(self):
        # Add padding to data to align it to 16 bytes.
        if len(self.data) % 16 != 0:
            self.data += b"\xff" * 16 - len(self.data) % 16

        # Add 16-byte "header" at the end with magic numbers and meta data.
        self.data += struct.pack(
            "<IHHHHI",
            FlashLayout.MAGIC1,
            FlashLayout.VERSION,
            len(self.data),
            self.num_regions,
            NRF_PAGE_SIZE_LOG2,
            FlashLayout.MAGIC2,
        )


def make_ihex_record(addr, type, data):
    record = struct.pack(">BHB", len(data), addr & 0xFFFF, type) + data
    checksum = (-(sum(record))) & 0xFF
    return ":%s%02X" % (str(binascii.hexlify(record), "utf8").upper(), checksum)


def parse_map_file(filename, symbols):
    parse_symbols = False
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if line == "Linker script and memory map":
                parse_symbols = True
            elif parse_symbols and line.startswith("0x00"):
                line = line.split()
                if len(line) >= 2 and line[1] in symbols:
                    symbols[line[1]] = int(line[0], 16)


def output_firmware(dest, firmware, layout_addr, layout_data):
    # Output head of firmware.
    for line in firmware[:-2]:
        print(line, end="", file=dest)

    # Output layout data.
    print(
        make_ihex_record(
            0,
            IHEX_TYPE_EXT_LIN_ADDR,
            struct.pack(">H", layout_addr >> 16),
        ),
        file=dest,
    )
    for i in range(0, len(layout_data), 16):
        chunk = layout_data[i : min(i + 16, len(layout_data))]
        print(
            make_ihex_record(layout_addr + i, IHEX_TYPE_DATA, chunk),
            file=dest,
        )

    # Output tail of firmware.
    print(firmware[-2], end="", file=dest)
    print(firmware[-1], end="", file=dest)


def main():
    arg_parser = argparse.ArgumentParser(
        description="Add UICR region to hex firmware for the micro:bit."
    )
    arg_parser.add_argument(
        "-o",
        "--output",
        default=sys.stdout,
        type=argparse.FileType("wt"),
        help="output file (default is stdout)",
    )
    arg_parser.add_argument("firmware", nargs=1, help="input MicroPython firmware")
    arg_parser.add_argument(
        "mapfile",
        nargs=1,
        help="input map file",
    )
    args = arg_parser.parse_args()

    # Read in the firmware from the given hex file.
    with open(args.firmware[0], "rt") as f:
        firmware = f.readlines()

    # Parse the linker map file, looking for the following symbols.
    symbols = {
        key: None
        for key in [
            "_binary_softdevice_bin_start",
            "__isr_vector",
            "__etext",
            "__data_start__",
            "__data_end__",
            "_fs_start",
            "_fs_end",
            "microbit_version_string",
        ]
    }
    parse_map_file(args.mapfile[0], symbols)

    # Get the required symbol addresses.
    sd_start = symbols["_binary_softdevice_bin_start"]
    sd_end = symbols["__isr_vector"]
    mp_start = symbols["__isr_vector"]
    data_len = symbols["__data_end__"] - symbols["__data_start__"]
    mp_end = symbols["__etext"] + data_len
    mp_version = symbols["microbit_version_string"]
    fs_start = symbols["_fs_start"]
    fs_end = symbols["_fs_end"]

    # Make the flash layout information table.
    layout = FlashLayout()
    layout.add_region(1, sd_start, sd_end - sd_start, FlashLayout.REGION_HASH_NONE)
    layout.add_region(
        2, mp_start, mp_end - mp_start, FlashLayout.REGION_HASH_PTR, mp_version
    )
    layout.add_region(3, fs_start, fs_end - fs_start, FlashLayout.REGION_HASH_NONE)
    layout.finalise()

    # Compute layout address.
    layout_addr = (
        ((mp_end >> NRF_PAGE_SIZE_LOG2) << NRF_PAGE_SIZE_LOG2)
        + NRF_PAGE_SIZE
        - len(layout.data)
    )
    if layout_addr < mp_end:
        layout_addr += NRF_PAGE_SIZE
    if layout_addr >= fs_start:
        print("ERROR: Flash layout information overlaps with filesystem")
        sys.exit(1)

    # Print information.
    if args.output is not sys.stdout:
        fmt = "{:13} 0x{:05x}..0x{:05x}"
        print(fmt.format("SoftDevice", sd_start, sd_end))
        print(fmt.format("MicroPython", mp_start, mp_end))
        print(fmt.format("Layout table", layout_addr, layout_addr + len(layout.data)))
        print(fmt.format("Filesystem", fs_start, fs_end))

    # Output the new firmware as a hex file.
    output_firmware(args.output, firmware, layout_addr, layout.data)


if __name__ == "__main__":
    main()
