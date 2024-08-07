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

mp_obj_t microbit_audio_track_new(mp_obj_t buffer_obj, size_t len, uint8_t *data, uint32_t rate) {
    microbit_audio_track_obj_t *self = m_new_obj(microbit_audio_track_obj_t);
    if (buffer_obj == MP_OBJ_NULL) {
        self->base.type = &microbit_audio_recording_type;
    } else {
        self->base.type = &microbit_audio_track_type;
        if (mp_obj_is_type(buffer_obj, &microbit_audio_track_type)) {
            buffer_obj = ((microbit_audio_track_obj_t *)MP_OBJ_TO_PTR(buffer_obj))->buffer_obj;
        }
    }
    self->buffer_obj = buffer_obj;
    self->size = len;
    self->rate = rate;
    self->data = data;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t microbit_audio_track_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    (void)type_in;

    enum { ARG_buffer, ARG_rate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buffer, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_OBJ_NULL} },
        { MP_QSTR_rate, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(AUDIO_TRACK_DEFAULT_SAMPLE_RATE)} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t rate = mp_obj_get_int_allow_float(args[ARG_rate].u_obj);
    if (rate <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("rate out of bounds"));
    }

    // Get buffer.
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buffer].u_obj, &bufinfo, MP_BUFFER_RW);

    // Create and return the AudioTrack object.
    return microbit_audio_track_new(args[ARG_buffer].u_obj, bufinfo.len, bufinfo.buf, rate);
}

static mp_obj_t microbit_audio_track_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(self->size);
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_obj_t microbit_audio_track_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value_in) {
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (value_in == MP_OBJ_NULL) {
        // delete
        mp_raise_TypeError(MP_ERROR_TEXT("cannot delete elements of AudioTrack"));
    } else if (value_in == MP_OBJ_SENTINEL) {
        // load
        if (mp_obj_is_type(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(self->size, index, &slice)) {
                mp_raise_NotImplementedError(MP_ERROR_TEXT("slices must have step=1"));
            }
            return microbit_audio_track_new(self->buffer_obj, slice.stop - slice.start, self->data + slice.start, self->rate);
        } else {
            size_t index_val = mp_get_index(self->base.type, self->size, index, false);
            return MP_OBJ_NEW_SMALL_INT(self->data[index_val]);
        }
    } else {
        // store
        size_t index_val = mp_get_index(self->base.type, self->size, index, false);
        mp_int_t value = mp_obj_get_int(value_in);
        if (value < 0 || value > 255) {
            mp_raise_ValueError(MP_ERROR_TEXT("value out of range"));
        }
        self->data[index_val] = value;
        return mp_const_none;
    }
}

mp_int_t microbit_audio_track_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->data;
    bufinfo->len = self->size;
    bufinfo->typecode = 'B';
    return 0;
}

static mp_obj_t microbit_audio_track_get_rate(mp_obj_t self_in) {
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->rate);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_audio_track_get_rate_obj, microbit_audio_track_get_rate);

static mp_obj_t microbit_audio_track_set_rate(mp_obj_t self_in, mp_obj_t rate_in) {
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rate = mp_obj_get_int_allow_float(rate_in);
    if (rate <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("rate out of bounds"));
    }
    self->rate = rate;
    // TODO: only set if this frame is currently being played
    microbit_hal_audio_raw_set_rate(rate);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_audio_track_set_rate_obj, microbit_audio_track_set_rate);

static mp_obj_t microbit_audio_track_copyfrom(mp_obj_t self_in, mp_obj_t other) {
    microbit_audio_track_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(other, &bufinfo, MP_BUFFER_READ);
    uint32_t len = MIN(bufinfo.len, self->size);
    memcpy(self->data, bufinfo.buf, len);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(microbit_audio_track_copyfrom_obj, microbit_audio_track_copyfrom);

static const mp_rom_map_elem_t microbit_audio_track_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_rate), MP_ROM_PTR(&microbit_audio_track_get_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rate), MP_ROM_PTR(&microbit_audio_track_set_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_copyfrom), MP_ROM_PTR(&microbit_audio_track_copyfrom_obj) },
};
static MP_DEFINE_CONST_DICT(microbit_audio_track_locals_dict, microbit_audio_track_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    microbit_audio_track_type,
    MP_QSTR_AudioTrack,
    MP_TYPE_FLAG_NONE,
    make_new, microbit_audio_track_make_new,
    unary_op, microbit_audio_track_unary_op,
    subscr, microbit_audio_track_subscr,
    buffer, microbit_audio_track_get_buffer,
    locals_dict, &microbit_audio_track_locals_dict
    );
