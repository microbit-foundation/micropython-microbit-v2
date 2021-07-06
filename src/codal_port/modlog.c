/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
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
#include "py/objstr.h"
#include "py/mphal.h"

STATIC mp_obj_t log_set_labels(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_timestamp };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_timestamp, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(MICROBIT_HAL_LOG_TIMESTAMP_MILLISECONDS)} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(0, NULL, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_timestamp].u_obj == mp_const_none) {
        microbit_hal_log_set_timestamp(MICROBIT_HAL_LOG_TIMESTAMP_NONE);
    } else {
        microbit_hal_log_set_timestamp(mp_obj_get_int(args[ARG_timestamp].u_obj));
    }

    if (n_args > 0) {
        // Create a row with empty values, which should just add a
        // heading row to the log data with the given labels/keys.
        microbit_hal_log_begin_row();
        for (size_t i = 0; i < n_args; i++) {
            const char *key = mp_obj_str_get_str(pos_args[i]);
            microbit_hal_log_data(key, "");
        }
        microbit_hal_log_end_row();
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(log_set_labels_obj, 0, log_set_labels);

STATIC mp_obj_t log_set_mirroring(mp_obj_t serial) {
    microbit_hal_log_set_mirroring(mp_obj_is_true(serial));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(log_set_mirroring_obj, log_set_mirroring);

STATIC mp_obj_t log_delete(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_full };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_full, MP_ARG_BOOL, {.u_bool = false} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(0, NULL, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    microbit_hal_log_delete(args[ARG_full].u_bool);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(log_delete_obj, 0, log_delete);

STATIC mp_obj_t log_add(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Get the dict to add as a row.
    mp_map_t *map;
    if (n_args == 0) {
        map = kw_args;
    } else if (n_args == 1) {
        if (!mp_obj_is_dict_or_ordereddict(pos_args[0])) {
            mp_raise_ValueError(MP_ERROR_TEXT("expecting a dict"));
        }
        map = mp_obj_dict_get_map(pos_args[0]);
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("too many arguments"));
    }

    // Add the log row.
    microbit_hal_log_begin_row();
    for (size_t i = 0; i < map->alloc; i++) {
        if (mp_map_slot_is_filled(map, i)) {
            // Get key, which should be a string.
            const char *key_str = mp_obj_str_get_str(map->table[i].key);

            // Convert value to string from an integer or float if applicable.
            mp_obj_t value = map->table[i].value;
            if (mp_obj_is_integer(value) || mp_obj_is_float(value)) {
                value = mp_obj_str_make_new(&mp_type_str, 1, 0, &value);
            }
            const char *value_str = mp_obj_str_get_str(value);

            // Add log entry.
            microbit_hal_log_data(key_str, value_str);
        }
    }
    microbit_hal_log_end_row();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(log_add_obj, 0, log_add);

STATIC const mp_rom_map_elem_t log_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_log) },
    { MP_ROM_QSTR(MP_QSTR_set_labels), MP_ROM_PTR(&log_set_labels_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_mirroring), MP_ROM_PTR(&log_set_mirroring_obj) },
    { MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&log_delete_obj) },
    { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&log_add_obj) },

    { MP_ROM_QSTR(MP_QSTR_MILLISECONDS), MP_ROM_INT(MICROBIT_HAL_LOG_TIMESTAMP_MILLISECONDS) },
    { MP_ROM_QSTR(MP_QSTR_SECONDS), MP_ROM_INT(MICROBIT_HAL_LOG_TIMESTAMP_SECONDS) },
    { MP_ROM_QSTR(MP_QSTR_MINUTES), MP_ROM_INT(MICROBIT_HAL_LOG_TIMESTAMP_MINUTES) },
    { MP_ROM_QSTR(MP_QSTR_HOURS), MP_ROM_INT(MICROBIT_HAL_LOG_TIMESTAMP_HOURS) },
    { MP_ROM_QSTR(MP_QSTR_DAYS), MP_ROM_INT(MICROBIT_HAL_LOG_TIMESTAMP_DAYS) },
};
STATIC MP_DEFINE_CONST_DICT(log_module_globals, log_module_globals_table);

const mp_obj_module_t log_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&log_module_globals,
};
