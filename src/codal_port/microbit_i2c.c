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

#include "py/mperrno.h"
#include "py/runtime.h"
#include "modmicrobit.h"
#include "microbithal.h"

typedef struct _microbit_i2c_obj_t {
    mp_obj_base_t base;
    const microbit_pin_obj_t *scl;
    const microbit_pin_obj_t *sda;
} microbit_i2c_obj_t;

STATIC mp_obj_t microbit_i2c_init(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_freq, ARG_sda, ARG_scl };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_sda, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE } },
        { MP_QSTR_scl, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE } },
    };

    // Parse arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get pins.
    const microbit_pin_obj_t *sda = &microbit_p20_obj;
    const microbit_pin_obj_t *scl = &microbit_p19_obj;
    if (args[ARG_sda].u_obj != mp_const_none) {
        sda = microbit_obj_get_pin(args[ARG_sda].u_obj);
    }
    if (args[ARG_scl].u_obj != mp_const_none) {
        scl = microbit_obj_get_pin(args[ARG_scl].u_obj);
    }

    // Acquire new pins and free the previous ones.
    microbit_obj_pin_acquire_and_free(&microbit_i2c_obj.scl, scl, microbit_pin_mode_i2c);
    microbit_obj_pin_acquire_and_free(&microbit_i2c_obj.sda, sda, microbit_pin_mode_i2c);

    // Initialise the I2C bus.
    int ret = microbit_hal_i2c_init(scl->name, sda->name, args[ARG_freq].u_int);

    if (ret != 0) {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_i2c_init_obj, 1, microbit_i2c_init);

STATIC mp_obj_t microbit_i2c_scan(mp_obj_t self_in) {
    mp_obj_t list = mp_obj_new_list(0, NULL);
    // 7-bit addresses 0b0000xxx and 0b1111xxx are reserved
    for (int addr = 0x08; addr < 0x78; ++addr) {
        int ret = microbit_hal_i2c_writeto(addr, NULL, 0, true);
        if (ret == 0) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }
    return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_i2c_scan_obj, microbit_i2c_scan);

STATIC mp_obj_t microbit_i2c_read(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_n, ARG_repeat };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_n,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_repeat,   MP_ARG_BOOL, {.u_bool = false} },
    };

    // Parse arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Prepare buffer to read data into.
    vstr_t vstr;
    vstr_init_len(&vstr, args[ARG_n].u_int);

    // Do the I2C read
    int err = microbit_hal_i2c_readfrom(args[ARG_addr].u_int, (uint8_t *)vstr.buf, vstr.len, !args[ARG_repeat].u_bool);
    if (err != 0) {
        // Assume an error means there is no I2C device with addr.
        mp_raise_OSError(MP_ENODEV);
    }

    // Return bytes object with read data.
    return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_i2c_read_obj, 1, microbit_i2c_read);

STATIC mp_obj_t microbit_i2c_write(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_buf, ARG_repeat };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_buf,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_repeat,   MP_ARG_BOOL, {.u_bool = false} },
    };

    // Parse arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Extract the buffer to write.
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);

    // Do the I2C write.
    int err = microbit_hal_i2c_writeto(args[ARG_addr].u_int, bufinfo.buf, bufinfo.len, !args[ARG_repeat].u_bool);
    if (err != 0) {
        // Assume an error means there is no I2C device with addr.
        mp_raise_OSError(MP_ENODEV);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_i2c_write_obj, 1, microbit_i2c_write);

STATIC const mp_rom_map_elem_t microbit_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&microbit_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&microbit_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&microbit_i2c_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&microbit_i2c_write_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_i2c_locals_dict, microbit_i2c_locals_dict_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    microbit_i2c_type,
    MP_QSTR_MicroBitI2C,
    MP_TYPE_FLAG_NONE,
    locals_dict, &microbit_i2c_locals_dict
    );

microbit_i2c_obj_t microbit_i2c_obj = {
    { &microbit_i2c_type },
    .scl = &microbit_p19_obj,
    .sda = &microbit_p20_obj,
};
