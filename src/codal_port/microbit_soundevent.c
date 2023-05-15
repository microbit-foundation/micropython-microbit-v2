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
#include "modmicrobit.h"

struct _microbit_soundevent_obj_t {
    mp_obj_base_t base;
    qstr name;
};

const microbit_soundevent_obj_t microbit_soundevent_loud_obj = {
    { &microbit_soundevent_type },
    MP_QSTR_loud,
};

const microbit_soundevent_obj_t microbit_soundevent_quiet_obj = {
    { &microbit_soundevent_type },
    MP_QSTR_quiet,
};

STATIC void microbit_soundevent_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    microbit_soundevent_obj_t *self = (microbit_soundevent_obj_t *)self_in;
    mp_printf(print, "SoundEvent('%q')", self->name);
}

STATIC const mp_rom_map_elem_t microbit_soundevent_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_LOUD), MP_ROM_PTR(&microbit_soundevent_loud_obj) },
    { MP_ROM_QSTR(MP_QSTR_QUIET), MP_ROM_PTR(&microbit_soundevent_quiet_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_soundevent_locals_dict, microbit_soundevent_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    microbit_soundevent_type,
    MP_QSTR_MicroBitSoundEvent,
    MP_TYPE_FLAG_NONE,
    print, microbit_soundevent_print,
    locals_dict, &microbit_soundevent_locals_dict
    );
