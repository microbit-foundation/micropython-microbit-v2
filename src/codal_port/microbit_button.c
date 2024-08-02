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

typedef struct _microbit_button_obj_t {
    mp_obj_base_t base;
    const microbit_pin_obj_t *pin;
    uint8_t button_id;
} microbit_button_obj_t;

mp_obj_t microbit_button_is_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t *)self_in;
    return mp_obj_new_bool(microbit_hal_button_state(self->button_id, NULL, NULL));
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_is_pressed_obj, microbit_button_is_pressed);

mp_obj_t microbit_button_get_presses(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t *)self_in;
    int n;
    microbit_hal_button_state(self->button_id, NULL, &n);
    return MP_OBJ_NEW_SMALL_INT(n);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_get_presses_obj, microbit_button_get_presses);

mp_obj_t microbit_button_was_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t *)self_in;
    int n;
    microbit_hal_button_state(self->button_id, &n, NULL);
    return mp_obj_new_bool(n);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_pressed_obj, microbit_button_was_pressed);

static const mp_map_elem_t microbit_button_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_pressed), (mp_obj_t)&microbit_button_is_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_pressed), (mp_obj_t)&microbit_button_was_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_presses), (mp_obj_t)&microbit_button_get_presses_obj },
};

static MP_DEFINE_CONST_DICT(microbit_button_locals_dict, microbit_button_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    microbit_button_type,
    MP_QSTR_MicroBitButton,
    MP_TYPE_FLAG_NONE,
    locals_dict, &microbit_button_locals_dict
    );

const microbit_button_obj_t microbit_button_a_obj = {
    .base = { &microbit_button_type },
    .pin = &microbit_p5_obj,
    .button_id = 0,
};

const microbit_button_obj_t microbit_button_b_obj = {
    .base = { &microbit_button_type },
    .pin = &microbit_p11_obj,
    .button_id = 1,
};

// This function assumes "button" is of type microbit_button_type.
uint8_t microbit_obj_get_button_id(mp_obj_t button) {
    return ((microbit_button_obj_t *)MP_OBJ_TO_PTR(button))->button_id;
}
