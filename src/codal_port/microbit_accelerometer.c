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

#include "py/runtime.h"
#include "py/mphal.h"
#include "modmicrobit.h"

typedef struct _microbit_accelerometer_obj_t {
    mp_obj_base_t base;
} microbit_accelerometer_obj_t;

mp_obj_t microbit_accelerometer_get_x(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_accelerometer_get_sample(axis);
    return mp_obj_new_int(axis[0]);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_x_obj, microbit_accelerometer_get_x);

mp_obj_t microbit_accelerometer_get_y(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_accelerometer_get_sample(axis);
    return mp_obj_new_int(axis[1]);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_y_obj, microbit_accelerometer_get_y);

mp_obj_t microbit_accelerometer_get_z(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_accelerometer_get_sample(axis);
    return mp_obj_new_int(axis[2]);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_z_obj, microbit_accelerometer_get_z);

mp_obj_t microbit_accelerometer_get_values(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_accelerometer_get_sample(axis);
    mp_obj_t tuple[3] = {
        mp_obj_new_int(axis[0]),
        mp_obj_new_int(axis[1]),
        mp_obj_new_int(axis[2]),
    };
    return mp_obj_new_tuple(3, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_values_obj, microbit_accelerometer_get_values);

STATIC const mp_rom_map_elem_t microbit_accelerometer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_x), MP_ROM_PTR(&microbit_accelerometer_get_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_y), MP_ROM_PTR(&microbit_accelerometer_get_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_z), MP_ROM_PTR(&microbit_accelerometer_get_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_values), MP_ROM_PTR(&microbit_accelerometer_get_values_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_accelerometer_locals_dict, microbit_accelerometer_locals_dict_table);

const mp_obj_type_t microbit_accelerometer_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitAccelerometer,
    .locals_dict = (mp_obj_dict_t *)&microbit_accelerometer_locals_dict,
};

const microbit_accelerometer_obj_t microbit_accelerometer_obj = {
    { &microbit_accelerometer_type },
};
