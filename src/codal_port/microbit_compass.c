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
#include "microbithal.h"
#include "modmicrobit.h"

typedef struct _microbit_compass_obj_t {
    mp_obj_base_t base;
} microbit_compass_obj_t;

STATIC mp_obj_t microbit_compass_is_calibrated(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(microbit_hal_compass_is_calibrated());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_is_calibrated_obj, microbit_compass_is_calibrated);

STATIC mp_obj_t microbit_compass_calibrate(mp_obj_t self_in) {
    (void)self_in;
    microbit_hal_compass_calibrate();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_calibrate_obj, microbit_compass_calibrate);

STATIC mp_obj_t microbit_compass_clear_calibration(mp_obj_t self_in) {
    (void)self_in;
    microbit_hal_compass_clear_calibration();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_clear_calibration_obj, microbit_compass_clear_calibration);

STATIC mp_obj_t microbit_compass_heading(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_int(microbit_hal_compass_get_heading());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_heading_obj, microbit_compass_heading);

STATIC mp_obj_t microbit_compass_get_x(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_compass_get_sample(axis);
    return mp_obj_new_int(axis[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_x_obj, microbit_compass_get_x);

STATIC mp_obj_t microbit_compass_get_y(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_compass_get_sample(axis);
    return mp_obj_new_int(axis[1]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_y_obj, microbit_compass_get_y);

STATIC mp_obj_t microbit_compass_get_z(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    microbit_hal_compass_get_sample(axis);
    return mp_obj_new_int(axis[2]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_z_obj, microbit_compass_get_z);

STATIC mp_obj_t microbit_compass_get_field_strength(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_int(microbit_hal_compass_get_field_strength());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_field_strength_obj, microbit_compass_get_field_strength);

STATIC const mp_rom_map_elem_t microbit_compass_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_heading), MP_ROM_PTR(&microbit_compass_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_calibrated), MP_ROM_PTR(&microbit_compass_is_calibrated_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate), MP_ROM_PTR(&microbit_compass_calibrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_calibration), MP_ROM_PTR(&microbit_compass_clear_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_x), MP_ROM_PTR(&microbit_compass_get_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_y), MP_ROM_PTR(&microbit_compass_get_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_z), MP_ROM_PTR(&microbit_compass_get_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_field_strength), MP_ROM_PTR(&microbit_compass_get_field_strength_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_compass_locals_dict, microbit_compass_locals_dict_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    microbit_compass_type,
    MP_QSTR_MicroBitCompass,
    MP_TYPE_FLAG_NONE,
    locals_dict, &microbit_compass_locals_dict
    );

const microbit_compass_obj_t microbit_compass_obj = {
    { &microbit_compass_type },
};
