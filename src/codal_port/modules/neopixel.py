# NeoPixel driver for MicroPython
# MIT license; Copyright (c) 2016-2020 Damien P. George

from microbit import ws2812_write


class NeoPixel:
    ORDER = (1, 0, 2, 3)

    def __init__(self, pin, n, bpp=3):
        self.pin = pin
        self.n = n
        self.bpp = bpp
        self.buf = bytearray(n * bpp)

    def __setitem__(self, index, val):
        offset = index * self.bpp
        for i in range(self.bpp):
            self.buf[offset + self.ORDER[i]] = val[i]

    def __getitem__(self, index):
        offset = index * self.bpp
        return tuple(self.buf[offset + self.ORDER[i]] for i in range(self.bpp))

    def fill(self, color):
        for i in range(self.n):
            self[i] = color

    def write(self):
        ws2812_write(self.pin, self.buf)

    # microbit v1 methods

    def __len__(self):
        return self.n

    def clear(self):
        for i in range(len(self.buf)):
            self.buf[i] = 0
        self.write()

    show = write
