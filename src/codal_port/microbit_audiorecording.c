/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Damien P. George
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

#include "py/mphal.h"
#include "drv_system.h"
#include "modmicrobit.h"
#include "modaudio.h"
#include "utils.h"

mp_obj_t microbit_audio_recording_new(size_t num_bytes, uint32_t rate) {
    // Make sure size is non-zero.
    if (num_bytes == 0) {
        num_bytes = 1;
    }

    // Create and return the AudioRecording object.
    uint8_t *data = m_new(uint8_t, num_bytes);
    memset(data, 128, num_bytes);
    return microbit_audio_track_new(MP_OBJ_NULL, num_bytes, data, rate);
}

static mp_obj_t microbit_audio_recording_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    (void)type_in;

    enum { ARG_duration, ARG_rate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_duration, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_OBJ_NULL} },
        { MP_QSTR_rate, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(AUDIO_TRACK_DEFAULT_SAMPLE_RATE)} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t rate = mp_obj_get_int_allow_float(args[ARG_rate].u_obj);
    if (rate <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("rate out of bounds"));
    }

    mp_float_t duration_ms = mp_obj_get_float(args[ARG_duration].u_obj);
    if (duration_ms <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("duration out of bounds"));
    }
    size_t num_bytes = duration_ms * rate / 1000;

    return microbit_audio_recording_new(num_bytes, rate);
}

static mp_obj_t microbit_audio_recording_copy(mp_obj_t self_in) {
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t *data = m_new(uint8_t, self->size);
    memcpy(data, self->data, self->size);
    return microbit_audio_track_new(MP_OBJ_NULL, self->size, data, self->rate);
}
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_audio_recording_copy_obj, microbit_audio_recording_copy);

static mp_obj_t microbit_audio_recording_track(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_start_ms, ARG_end_ms };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_start_ms, MP_ARG_KW_ONLY, {.u_rom_obj = MP_ROM_INT(0)} },
        { MP_QSTR_end_ms, MP_ARG_KW_ONLY, {.u_rom_obj = MP_ROM_INT(-1)} },
    };
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_int_t start_byte = mp_obj_get_float(args[ARG_start_ms].u_obj) * self->rate / 1000;
    mp_int_t end_byte;
    if (args[ARG_end_ms].u_obj == MP_OBJ_NEW_SMALL_INT(-1)) {
        end_byte = self->size;
    } else {
        end_byte = mp_obj_get_float(args[ARG_end_ms].u_obj) * self->rate / 1000;
    }

    // Truncate start_byte to fit in valid range.
    start_byte = MAX(0, start_byte);
    start_byte = MIN(start_byte, self->size);

    // Truncate end_byte to fit in valid range.
    end_byte = MAX(0, end_byte);
    end_byte = MIN(end_byte, self->size);

    // Calculate length of track, truncating negative lengths to 0.
    size_t len = MAX(0, end_byte - start_byte);

    // Create and return new track.
    return microbit_audio_track_new(pos_args[0], len, self->data + start_byte, self->rate);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(microbit_audio_recording_track_obj, 1, microbit_audio_recording_track);

static const mp_rom_map_elem_t microbit_audio_recording_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_rate), MP_ROM_PTR(&microbit_audio_track_get_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rate), MP_ROM_PTR(&microbit_audio_track_set_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_copy), MP_ROM_PTR(&microbit_audio_recording_copy_obj) },
    { MP_ROM_QSTR(MP_QSTR_track), MP_ROM_PTR(&microbit_audio_recording_track_obj) },
};
static MP_DEFINE_CONST_DICT(microbit_audio_recording_locals_dict, microbit_audio_recording_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    microbit_audio_recording_type,
    MP_QSTR_AudioRecording,
    MP_TYPE_FLAG_NONE,
    make_new, microbit_audio_recording_make_new,
    buffer, microbit_audio_track_get_buffer,
    locals_dict, &microbit_audio_recording_locals_dict
    );
