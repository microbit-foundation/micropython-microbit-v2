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
#define AUDIO_TRACK_DEFAULT_SAMPLE_RATE (7812)

typedef struct _microbit_audio_frame_obj_t {
    mp_obj_base_t base;
    size_t alloc_size;
    uint32_t rate;
    uint8_t data[];
} microbit_audio_frame_obj_t;

typedef struct _microbit_audio_track_obj_t {
    mp_obj_base_t base;
    mp_obj_t buffer_obj; // so GC can't reclaim the underlying buffer
    size_t size;
    uint32_t rate;
    uint8_t *data;
} microbit_audio_track_obj_t;

extern const mp_obj_type_t microbit_audio_frame_type;
extern const mp_obj_type_t microbit_audio_recording_type;
extern const mp_obj_type_t microbit_audio_track_type;
extern const mp_obj_module_t audio_module;

void microbit_audio_data_add_inplace(uint8_t *lhs_data, const uint8_t *rhs_data, size_t size, bool add);
void microbit_audio_data_mult_inplace(uint8_t *data, size_t size, float f);

void microbit_audio_play_source(mp_obj_t src, mp_obj_t pin_select, bool wait, uint32_t sample_rate);
void microbit_audio_stop(void);
bool microbit_audio_is_playing(void);

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(size_t size, uint32_t rate);
mp_obj_t microbit_audio_track_new(mp_obj_t buffer_obj, size_t len, uint8_t *data, uint32_t rate);
mp_obj_t microbit_audio_recording_new(size_t num_bytes, uint32_t rate);

const char *microbit_soundeffect_get_sound_expr_data(mp_obj_t self_in);

mp_int_t microbit_audio_track_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);
MP_DECLARE_CONST_FUN_OBJ_1(microbit_audio_track_get_rate_obj);
MP_DECLARE_CONST_FUN_OBJ_2(microbit_audio_track_set_rate_obj);

#endif // MICROPY_INCLUDED_MICROBIT_MODAUDIO_H
