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

#define SOUND(name, ...) \
    STATIC const microbit_sound_obj_t microbit_sound_ ## name ## _obj = { \
        { &microbit_sound_type }, \
        MP_STRINGIFY(name) \
    }

SOUND(giggle);
SOUND(happy);
SOUND(hello);
SOUND(mysterious);
SOUND(sad);
SOUND(slide);
SOUND(soaring);
SOUND(spring);
SOUND(twinkle);
SOUND(yawn);

#undef SOUND

STATIC void microbit_sound_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    const microbit_sound_obj_t *self = (const microbit_sound_obj_t *)self_in;
    mp_printf(print, "Sound('%s')", self->name);
}

STATIC const mp_rom_map_elem_t microbit_sound_locals_dict_table[] = {
    #define SOUND(NAME, name) { MP_ROM_QSTR(MP_QSTR_ ## NAME), MP_ROM_PTR(&microbit_sound_ ## name ## _obj) }

    SOUND(GIGGLE, giggle),
    SOUND(HAPPY, happy),
    SOUND(HELLO, hello),
    SOUND(MYSTERIOUS, mysterious),
    SOUND(SAD, sad),
    SOUND(SLIDE, slide),
    SOUND(SOARING, soaring),
    SOUND(SPRING, spring),
    SOUND(TWINKLE, twinkle),
    SOUND(YAWN, yawn),

    #undef SOUND
};
STATIC MP_DEFINE_CONST_DICT(microbit_sound_locals_dict, microbit_sound_locals_dict_table);

const mp_obj_type_t microbit_sound_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitSound,
    .print = microbit_sound_print,
    .locals_dict = (mp_obj_dict_t *)&microbit_sound_locals_dict,
};
