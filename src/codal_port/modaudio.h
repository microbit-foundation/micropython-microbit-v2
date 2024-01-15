/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Mark Shannon
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
#ifndef MICROPY_INCLUDED_MICROBIT_MODAUDIO_H
#define MICROPY_INCLUDED_MICROBIT_MODAUDIO_H

#include "py/runtime.h"

#define SOUND_EXPR_TOTAL_LENGTH (72)

typedef struct _microbit_audio_frame_obj_t {
    mp_obj_base_t base;
    size_t alloc_size;
    size_t used_size;
    uint8_t data[];
} microbit_audio_frame_obj_t;

extern const mp_obj_type_t microbit_audio_frame_type;
extern const mp_obj_module_t audio_module;

void microbit_audio_play_source(mp_obj_t src, mp_obj_t pin_select, bool wait, uint32_t sample_rate);
void microbit_audio_stop(void);
bool microbit_audio_is_playing(void);

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(size_t size);

const char *microbit_soundeffect_get_sound_expr_data(mp_obj_t self_in);

#endif // MICROPY_INCLUDED_MICROBIT_MODAUDIO_H
