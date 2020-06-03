/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "main.h"

int last_interrupt_char;
unsigned int num_interrupt_chars;

extern "C" void microbit_hal_serial_interrupt_callback(void);

void serial_interrupt_handler(Event evt) {
    ++num_interrupt_chars;
    microbit_hal_serial_interrupt_callback();
}

extern "C" {

void mp_hal_set_interrupt_char(int c) {
    ManagedString delim;
    if (c != -1) {
        last_interrupt_char = c;
        delim = ManagedString((char)c);
    }
    uBit.serial.eventOn(delim);
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    uBit.serial.send((uint8_t*)str, len, SYNC_SPINWAIT);
}

int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int c = uBit.serial.read(SYNC_SPINWAIT);
        if (c == last_interrupt_char && num_interrupt_chars) {
            --num_interrupt_chars;
        } else {
            return c;
        }
    }
}

uint32_t mp_hal_ticks_us(void) {
    return system_timer_current_time_us();
}

uint32_t mp_hal_ticks_ms(void) {
    return system_timer_current_time();
}

}
