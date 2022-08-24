/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Damien P. George
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
#include "py/mphal.h"

STATIC size_t get_array(mp_obj_t *src, mp_obj_t **items) {
    if (*src == mp_const_none) {
        // None, so an array of length 0.
        *items = NULL;
        return 0;
    } else if (mp_obj_is_type(*src, &mp_type_tuple) || mp_obj_is_type(*src, &mp_type_list)) {
        // A tuple/list passed in, get items and length.
        size_t len;
        mp_obj_get_array(*src, &len, items);
        return len;
    } else {
        // A single object passed in, so an array of length 1.
        *items = src;
        return 1;
    }
}

STATIC mp_obj_t power_off(void) {
    microbit_hal_power_off();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(power_off_obj, power_off);

STATIC mp_obj_t power_deep_sleep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_ms, ARG_wake_on, ARG_run_every };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ms, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_wake_on, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_run_every, MP_ARG_BOOL, {.u_bool = false} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    microbit_hal_power_clear_wake_sources();

    // Configure wake-up sources.
    mp_obj_t *items;
    size_t len = get_array(&args[ARG_wake_on].u_obj, &items);
    for (size_t i = 0; i < len; ++i) {
        const mp_obj_type_t *type = mp_obj_get_type(items[i]);
        if (microbit_obj_type_is_button(type)) {
            microbit_hal_power_wake_on_button(microbit_obj_get_button_id(items[i]), true);
        } else if (microbit_obj_type_is_pin(type)) {
            microbit_hal_power_wake_on_pin(microbit_obj_get_pin_name(items[i]), true);
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("expecting a pin or button"));
        }
    }

    if (args[ARG_run_every].u_bool) {
        mp_raise_ValueError(MP_ERROR_TEXT("run_every not implemented"));
    }

    if (args[ARG_ms].u_obj == mp_const_none) {
        microbit_hal_power_deep_sleep(false, 0);
    } else {
        microbit_hal_power_deep_sleep(true, mp_obj_get_int(args[ARG_ms].u_obj));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(power_deep_sleep_obj, 0, power_deep_sleep);

STATIC const mp_rom_map_elem_t power_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_power) },

    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&power_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_deep_sleep), MP_ROM_PTR(&power_deep_sleep_obj) },
};
STATIC MP_DEFINE_CONST_DICT(power_module_globals, power_module_globals_table);

const mp_obj_module_t power_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&power_module_globals,
};
