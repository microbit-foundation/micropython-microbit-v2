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
#include "microbithal.h"
#include "modmicrobit.h"

typedef struct _microbit_speaker_obj_t {
    mp_obj_base_t base;
} microbit_speaker_obj_t;

STATIC mp_obj_t microbit_speaker_off(mp_obj_t self_in) {
    (void)self_in;
    microbit_hal_audio_select_speaker(false);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_speaker_off_obj, microbit_speaker_off);

STATIC mp_obj_t microbit_speaker_on(mp_obj_t self_in) {
    (void)self_in;
    microbit_hal_audio_select_speaker(true);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_speaker_on_obj, microbit_speaker_on);

STATIC const mp_rom_map_elem_t microbit_speaker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&microbit_speaker_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&microbit_speaker_on_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_speaker_locals_dict, microbit_speaker_locals_dict_table);

STATIC const mp_obj_type_t microbit_speaker_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitSpeakerPin,
    .locals_dict = (mp_obj_dict_t *)&microbit_speaker_locals_dict,
};

const microbit_speaker_obj_t microbit_speaker_obj = {
    { &microbit_speaker_type },
};
