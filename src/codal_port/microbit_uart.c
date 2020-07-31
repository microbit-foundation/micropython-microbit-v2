/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2020 Damien P. George
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

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "modmicrobit.h"

// There is only one UART peripheral and it's already used by stdio (and
// connected to USB serial).  So to access the UART we go through the
// mp_hal_stdio functions.  The init method can reconfigure the underlying
// UART peripheral to a different baudrate and/or pins.

typedef struct _microbit_uart_obj_t {
    mp_obj_base_t base;
} microbit_uart_obj_t;

// timeout (in ms) to wait between characters when reading
STATIC uint16_t microbit_uart_timeout_char = 0;

STATIC mp_obj_t microbit_uart_init(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop, ARG_pins, ARG_tx, ARG_rx };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 9600} },
        { MP_QSTR_bits,     MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity,   MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_stop,     MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_pins,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_tx,       MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_rx,       MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    // Parse arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int parity;
    if (args[ARG_parity].u_obj == mp_const_none) {
        parity = -1; // no parity
    } else {
        parity = mp_obj_get_int(args[ARG_parity].u_obj);
    }

    // Get pins.
    int tx = MICROBIT_HAL_PIN_USB_TX;
    int rx = MICROBIT_HAL_PIN_USB_RX;
    if (args[ARG_tx].u_obj != mp_const_none) {
        tx = microbit_obj_get_pin(args[ARG_tx].u_obj)->name;
    }
    if (args[ARG_rx].u_obj != mp_const_none) {
        rx = microbit_obj_get_pin(args[ARG_rx].u_obj)->name;
    }

    // Support for legacy "pins" argument.
    if (args[ARG_pins].u_obj != mp_const_none) {
        mp_obj_t *pins;
        mp_obj_get_array_fixed_n(args[ARG_pins].u_obj, 2, &pins);
        tx = microbit_obj_get_pin(pins[0])->name;
        rx = microbit_obj_get_pin(pins[1])->name;
    }

    // Initialise the uart.
    microbit_hal_uart_init(tx, rx, args[ARG_baudrate].u_int,
        args[ARG_bits].u_int, parity, args[ARG_stop].u_int);

    // Set the character read timeout based on the baudrate and 13 bits.
    microbit_uart_timeout_char = 13000 / args[0].u_int + 1;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_uart_init_obj, 1, microbit_uart_init);

STATIC mp_obj_t microbit_uart_any(mp_obj_t self_in) {
    (void)self_in;
    if (mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
        return mp_const_true;
    } else {
        return mp_const_false;
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_uart_any_obj, microbit_uart_any);

STATIC const mp_rom_map_elem_t microbit_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&microbit_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&microbit_uart_any_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },

    { MP_ROM_QSTR(MP_QSTR_ODD), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_EVEN), MP_ROM_INT(0) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_uart_locals_dict, microbit_uart_locals_dict_table);

// Waits at most timeout_ms for at least 1 char to become ready for reading.
// Returns true if something available, false if not.
STATIC bool microbit_uart_rx_wait(uint32_t timeout_ms) {
    uint32_t start = mp_hal_ticks_ms();
    for (;;) {
        if (mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
            return true; // have at least 1 character waiting
        }
        if (mp_hal_ticks_ms() - start >= timeout_ms) {
            return false; // timeout
        }
        mp_handle_pending(true);
    }
}

STATIC mp_uint_t microbit_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    (void)self_in;
    byte *buf = (byte*)buf_in;
    (void)errcode;

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    // check there is at least 1 char available
    if (!mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    byte *orig_buf = buf;
    for (;;) {
        *buf++ = mp_hal_stdin_rx_chr();
        if (--size == 0 || !microbit_uart_rx_wait(microbit_uart_timeout_char)) {
            // return number of bytes read
            return buf - orig_buf;
        }
    }
}

STATIC mp_uint_t microbit_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    (void)self_in;
    const char *buf = (const char*)buf_in;
    (void)errcode;
    mp_hal_stdout_tx_strn(buf, size);
    return size;
}

STATIC const mp_stream_p_t microbit_uart_stream_p = {
    .read = microbit_uart_read,
    .write = microbit_uart_write,
    .is_text = false,
};

STATIC const mp_obj_type_t microbit_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitUART,
    .protocol = &microbit_uart_stream_p,
    .locals_dict = (mp_obj_dict_t *)&microbit_uart_locals_dict,
};

const microbit_uart_obj_t microbit_uart_obj = {
    { &microbit_uart_type },
};
