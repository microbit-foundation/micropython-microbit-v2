/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2020 Damien P. George
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

#include <string.h>

#include "py/runtime.h"
#include "microbithal.h"
#include "modmicrobit.h"

typedef struct _microbit_spi_obj_t {
    mp_obj_base_t base;
    const microbit_pin_obj_t *sclk;
    const microbit_pin_obj_t *mosi;
    const microbit_pin_obj_t *miso;
} microbit_spi_obj_t;

STATIC bool microbit_spi_initialised = false;

STATIC void microbit_spi_check_initialised(void) {
    if (!microbit_spi_initialised) {
        mp_raise_ValueError(MP_ERROR_TEXT("SPI not initialised"));
    }
}

STATIC mp_obj_t microbit_spi_init(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_mode, ARG_sclk, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 1000000} },
        { MP_QSTR_bits,     MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_mode,     MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_sclk,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    // Parse arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get pins.
    const microbit_pin_obj_t *sclk = &microbit_p13_obj;
    const microbit_pin_obj_t *mosi = &microbit_p15_obj;
    const microbit_pin_obj_t *miso = &microbit_p14_obj;
    if (args[ARG_sclk].u_obj != mp_const_none) {
        sclk = microbit_obj_get_pin(args[ARG_sclk].u_obj);
    }
    if (args[ARG_mosi].u_obj != mp_const_none) {
        mosi = microbit_obj_get_pin(args[ARG_mosi].u_obj);
    }
    if (args[ARG_miso].u_obj != mp_const_none) {
        miso = microbit_obj_get_pin(args[ARG_miso].u_obj);
    }

    // Acquire new pins and free the previous ones.
    microbit_obj_pin_acquire_and_free(&microbit_spi_obj.sclk, sclk, microbit_pin_mode_spi);
    microbit_obj_pin_acquire_and_free(&microbit_spi_obj.mosi, mosi, microbit_pin_mode_spi);
    microbit_obj_pin_acquire_and_free(&microbit_spi_obj.miso, miso, microbit_pin_mode_spi);

    // Initialise the SPI bus.
    int ret = microbit_hal_spi_init(sclk->name, mosi->name, miso->name,
        args[ARG_baudrate].u_int, args[ARG_bits].u_int, args[ARG_mode].u_int);

    if (ret != 0) {
        mp_raise_OSError(ret);
    }

    microbit_spi_initialised = true;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_spi_init_obj, 1, microbit_spi_init);

STATIC mp_obj_t microbit_spi_write(mp_obj_t self_in, mp_obj_t buf_in) {
    microbit_spi_check_initialised();
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    microbit_hal_spi_transfer(bufinfo.len, bufinfo.buf, NULL);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_spi_write_obj, microbit_spi_write);

STATIC mp_obj_t microbit_spi_read(size_t n_args, const mp_obj_t *args) {
    microbit_spi_check_initialised();
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[1]));
    uint8_t byte_out = 0;
    if (n_args == 3) {
        byte_out = mp_obj_get_int(args[2]);
    }
    memset(vstr.buf, byte_out, vstr.len);
    int ret = microbit_hal_spi_transfer(vstr.len, (uint8_t *)vstr.buf, (uint8_t *)vstr.buf);
    if (ret != 0) {
        mp_raise_OSError(ret);
    }
    return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_spi_read_obj, 2, 3, microbit_spi_read);

STATIC mp_obj_t microbit_spi_write_readinto(mp_obj_t self_in, mp_obj_t write_buf, mp_obj_t read_buf) {
    microbit_spi_check_initialised();
    mp_buffer_info_t write_bufinfo;
    mp_get_buffer_raise(write_buf, &write_bufinfo, MP_BUFFER_READ);
    mp_buffer_info_t read_bufinfo;
    mp_get_buffer_raise(read_buf, &read_bufinfo, MP_BUFFER_WRITE);
    if (write_bufinfo.len != read_bufinfo.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("write and read buffers must be the same length"));
    }
    microbit_hal_spi_transfer(write_bufinfo.len, write_bufinfo.buf, read_bufinfo.buf);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(microbit_spi_write_readinto_obj, microbit_spi_write_readinto);

STATIC const mp_rom_map_elem_t microbit_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&microbit_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&microbit_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&microbit_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&microbit_spi_write_readinto_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_spi_locals_dict, microbit_spi_locals_dict_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    microbit_spi_type,
    MP_QSTR_MicroBitSPI,
    MP_TYPE_FLAG_NONE,
    locals_dict, &microbit_spi_locals_dict
    );

microbit_spi_obj_t microbit_spi_obj = {
    { &microbit_spi_type },
    .sclk = &microbit_p13_obj,
    .mosi = &microbit_p15_obj,
    .miso = &microbit_p14_obj,
};
