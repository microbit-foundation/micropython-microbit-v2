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

#include "py/mphal.h"
#include "drv_system.h"
#include "modaudio.h"
#include "modmicrobit.h"

// Convenience macros to access the root-pointer state.
#define audio_source_frame MP_STATE_PORT(audio_source_frame_state)
#define audio_source_iter MP_STATE_PORT(audio_source_iter_state)

#define LOG_AUDIO_CHUNK_SIZE (5)
#define AUDIO_CHUNK_SIZE (1 << LOG_AUDIO_CHUNK_SIZE)
#define DEFAULT_SAMPLE_RATE (7812)

typedef enum {
    AUDIO_OUTPUT_STATE_IDLE,
    AUDIO_OUTPUT_STATE_DATA_READY,
    AUDIO_OUTPUT_STATE_DATA_WRITTEN,
} audio_output_state_t;

static uint8_t audio_output_buffer[AUDIO_CHUNK_SIZE];
static volatile audio_output_state_t audio_output_state;
static volatile bool audio_fetcher_scheduled;
static size_t audio_raw_offset;

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(size_t size);

static inline bool audio_is_running(void) {
    return audio_source_frame != NULL || audio_source_iter != MP_OBJ_NULL;
}

void microbit_audio_stop(void) {
    audio_source_frame = NULL;
    audio_source_iter = NULL;
    audio_raw_offset = 0;
    microbit_hal_audio_stop_expression();
}

static void audio_buffer_ready(void) {
    uint32_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    audio_output_state_t old_state = audio_output_state;
    audio_output_state = AUDIO_OUTPUT_STATE_DATA_READY;
    MICROPY_END_ATOMIC_SECTION(atomic_state);
    if (old_state == AUDIO_OUTPUT_STATE_IDLE) {
        microbit_hal_audio_raw_ready_callback();
    }
}

static void audio_data_fetcher(void) {
    audio_fetcher_scheduled = false;

    if (audio_source_frame != NULL) {
        // An existing AudioFrame is being played, see if there's any data left.
        if (audio_raw_offset >= audio_source_frame->size) {
            // AudioFrame is exhausted.
            audio_source_frame = NULL;
        }
    }

    if (audio_source_frame == NULL) {
        // There is no AudioFrame, so try to get one from the audio iterator.

        if (audio_source_iter == MP_OBJ_NULL) {
            // Audio iterator is already exhausted.
            microbit_audio_stop();
            return;
        }

        // Get the next item from the audio iterator.
        nlr_buf_t nlr;
        mp_obj_t frame_obj;
        if (nlr_push(&nlr) == 0) {
            frame_obj = mp_iternext_allow_raise(audio_source_iter);
            nlr_pop();
        } else {
            if (!mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t*)nlr.ret_val)->type),
                MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
                mp_sched_exception(MP_OBJ_FROM_PTR(nlr.ret_val));
            }
            frame_obj = MP_OBJ_STOP_ITERATION;
        }
        if (frame_obj == MP_OBJ_STOP_ITERATION) {
            // End of audio iterator.
            microbit_audio_stop();
            return;
        }
        if (!mp_obj_is_type(frame_obj, &microbit_audio_frame_type)) {
            // Audio iterator did not return an AudioFrame.
            microbit_audio_stop();
            mp_sched_exception(mp_obj_new_exception_msg(&mp_type_TypeError, MP_ERROR_TEXT("not an AudioFrame")));
            return;
        }

        // We have the next AudioFrame.
        audio_source_frame = MP_OBJ_TO_PTR(frame_obj);
        audio_raw_offset = 0;
    }

    const uint8_t *src = &audio_source_frame->data[audio_raw_offset];
    audio_raw_offset += AUDIO_CHUNK_SIZE;

    uint8_t *dest = &audio_output_buffer[0];
    for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i) {
        // Copy sample to the buffer.
        *dest++ = src[i];
    }

    audio_buffer_ready();
}

static mp_obj_t audio_data_fetcher_wrapper(mp_obj_t arg) {
    audio_data_fetcher();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(audio_data_fetcher_wrapper_obj, audio_data_fetcher_wrapper);

void microbit_hal_audio_raw_ready_callback(void) {
    if (audio_output_state == AUDIO_OUTPUT_STATE_DATA_READY) {
        // there is data ready to send out to the audio pipeline, so send it
        microbit_hal_audio_raw_write_data(&audio_output_buffer[0], AUDIO_CHUNK_SIZE);
        audio_output_state = AUDIO_OUTPUT_STATE_DATA_WRITTEN;
    } else {
        // no data ready, need to call this function later when data is ready
        audio_output_state = AUDIO_OUTPUT_STATE_IDLE;
    }
    if (!audio_fetcher_scheduled) {
        // schedule audio_data_fetcher to be executed to prepare the next buffer
        audio_fetcher_scheduled = mp_sched_schedule(MP_OBJ_FROM_PTR(&audio_data_fetcher_wrapper_obj), mp_const_none);
    }
}

static void audio_init(uint32_t sample_rate) {
    audio_fetcher_scheduled = false;
    audio_output_state = AUDIO_OUTPUT_STATE_IDLE;
    microbit_hal_audio_raw_init(sample_rate);
}

void microbit_audio_play_source(mp_obj_t src, mp_obj_t pin_select, bool wait, uint32_t sample_rate) {
    if (audio_is_running()) {
        microbit_audio_stop();
    }
    audio_init(sample_rate);
    microbit_pin_audio_select(pin_select, microbit_pin_mode_audio_play);

    const char *sound_expr_data = NULL;
    if (mp_obj_is_type(src, &microbit_sound_type)) {
        const microbit_sound_obj_t *sound = (const microbit_sound_obj_t *)MP_OBJ_TO_PTR(src);
        sound_expr_data = sound->name;
    } else if (mp_obj_is_type(src, &microbit_soundeffect_type)) {
        sound_expr_data = microbit_soundeffect_get_sound_expr_data(src);
    } else if (mp_obj_is_type(src, &microbit_audio_frame_type)) {
        audio_source_frame = MP_OBJ_TO_PTR(src);
        audio_raw_offset = 0;
    } else if (mp_obj_is_type(src, &mp_type_tuple) || mp_obj_is_type(src, &mp_type_list)) {
        // A tuple/list passed in.  Need to check if it contains SoundEffect instances.
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(src, &len, &items);
        if (len > 0 && mp_obj_is_type(items[0], &microbit_soundeffect_type)) {
            // A tuple/list of SoundEffect instances.  Convert it to a long string of
            // sound expression data, each effect separated by a ",".
            char *data = m_new(char, len * (SOUND_EXPR_TOTAL_LENGTH + 1));
            sound_expr_data = data;
            for (size_t i = 0; i < len; ++i) {
                memcpy(data, microbit_soundeffect_get_sound_expr_data(items[i]), SOUND_EXPR_TOTAL_LENGTH);
                data += SOUND_EXPR_TOTAL_LENGTH;
                *data++ = ',';
            }
            // Replace last "," with a string null terminator.
            data[-1] = '\0';
        } else {
            // A tuple/list of AudioFrame instances.
            audio_source_iter = mp_getiter(src, NULL);
        }
    } else {
        // An iterator of AudioFrame instances.
        audio_source_iter = mp_getiter(src, NULL);
    }

    if (sound_expr_data != NULL) {
        microbit_hal_audio_play_expression(sound_expr_data);
        if (wait) {
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                // Wait for the expression to finish playing.
                while (microbit_hal_audio_is_expression_active()) {
                    mp_handle_pending(true);
                    microbit_hal_idle();
                }
                nlr_pop();
            } else {
                // Catch all exceptions and stop the audio before re-raising.
                microbit_hal_audio_stop_expression();
                nlr_jump(nlr.ret_val);
            }
        }
        return;
    }

    // Start the audio running.
    // The scheduler must be locked because audio_data_fetcher() can also be called from the scheduler.
    mp_sched_lock();
    audio_data_fetcher();
    mp_sched_unlock();

    if (wait) {
        // Wait the audio to exhaust the iterator.
        while (audio_is_running()) {
            mp_handle_pending(true);
            microbit_hal_idle();
        }
    }
}

static mp_obj_t stop(void) {
    microbit_audio_stop();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_audio_stop_obj, stop);

static mp_obj_t play(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Note: the return_pin argument is for compatibility with micro:bit v1 and is ignored on v2.
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_source, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_wait,  MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_pin,   MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&microbit_pin_default_audio_obj)} },
        { MP_QSTR_return_pin,   MP_ARG_OBJ, {.u_obj = mp_const_none } },
    };
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t src = args[0].u_obj;
    microbit_audio_play_source(src, args[2].u_obj, args[1].u_bool, DEFAULT_SAMPLE_RATE);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_audio_play_obj, 0, play);

bool microbit_audio_is_playing(void) {
    return audio_is_running() || microbit_hal_audio_is_expression_active();
}

mp_obj_t is_playing(void) {
    return mp_obj_new_bool(microbit_audio_is_playing());
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_audio_is_playing_obj, is_playing);

static const mp_rom_map_elem_t audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&microbit_audio_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&microbit_audio_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_playing), MP_ROM_PTR(&microbit_audio_is_playing_obj) },
    { MP_ROM_QSTR(MP_QSTR_AudioFrame), MP_ROM_PTR(&microbit_audio_frame_type) },
    { MP_ROM_QSTR(MP_QSTR_SoundEffect), MP_ROM_PTR(&microbit_soundeffect_type) },
};
static MP_DEFINE_CONST_DICT(audio_module_globals, audio_globals_table);

const mp_obj_module_t audio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&audio_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_audio, audio_module);

/******************************************************************************/
// AudioFrame class

static mp_obj_t microbit_audio_frame_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    (void)args;
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    size_t size = AUDIO_CHUNK_SIZE;
    if (n_args == 1) {
        size = mp_obj_get_int(args[0]);
        if (size <= 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("size out of bounds"));
        }
    }
    return microbit_audio_frame_make_new(size);
}

static mp_obj_t audio_frame_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value_in) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    mp_int_t index = mp_obj_get_int(index_in);
    if (index < 0 || index >= self->size) {
         mp_raise_ValueError(MP_ERROR_TEXT("index out of bounds"));
    }
    if (value_in == MP_OBJ_NULL) {
        // delete
        mp_raise_TypeError(MP_ERROR_TEXT("cannot delete elements of AudioFrame"));
    } else if (value_in == MP_OBJ_SENTINEL) {
        // load
        return MP_OBJ_NEW_SMALL_INT(self->data[index]);
    } else {
        mp_int_t value = mp_obj_get_int(value_in);
        if (value < 0 || value > 255) {
            mp_raise_ValueError(MP_ERROR_TEXT("value out of range"));
        }
        self->data[index] = value;
        return mp_const_none;
    }
}

static mp_obj_t audio_frame_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    switch (op) {
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(self->size);
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_int_t audio_frame_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    bufinfo->buf = self->data;
    bufinfo->len = self->size;
    bufinfo->typecode = 'b';
    return 0;
}

static void add_into(microbit_audio_frame_obj_t *self, microbit_audio_frame_obj_t *other, bool add) {
    int mult = add ? 1 : -1;
    size_t size = MIN(self->size, other->size);
    for (int i = 0; i < size; i++) {
        unsigned val = (int)self->data[i] + mult*(other->data[i]-128);
        // Clamp to 0-255
        if (val > 255) {
            val = (1-(val>>31))*255;
        }
        self->data[i] = val;
    }
}

static microbit_audio_frame_obj_t *copy(microbit_audio_frame_obj_t *self) {
    microbit_audio_frame_obj_t *result = microbit_audio_frame_make_new(self->size);
    for (int i = 0; i < self->size; i++) {
        result->data[i] = self->data[i];
    }
    return result;
}

mp_obj_t copyfrom(mp_obj_t self_in, mp_obj_t other) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(other, &bufinfo, MP_BUFFER_READ);
    uint32_t len = MIN(bufinfo.len, self->size);
    for (uint32_t i = 0; i < len; i++) {
        self->data[i] = ((uint8_t *)bufinfo.buf)[i];
    }
   return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(copyfrom_obj, copyfrom);

union _i2f {
    int32_t bits;
    float value;
};

/* Convert a small float to a fixed-point number */
int32_t float_to_fixed(float f, uint32_t scale) {
    union _i2f x;
    x.value = f;
    int32_t sign = 1-((x.bits>>30)&2);
    /* Subtract 127 from exponent for IEEE-754 and 23 for mantissa scaling */
    int32_t exponent = ((x.bits>>23)&255)-150;
    /* Mantissa scaled by 2**23, including implicit 1 */
    int32_t mantissa = (1<<23) | ((x.bits)&((1<<23)-1));
    int32_t shift = scale+exponent;
    int32_t result;
    if (shift > 0) {
        result = sign*(mantissa<<shift);
    } else if (shift < -31) {
        result = 0;
    } else {
        result = sign*(mantissa>>(-shift));
    }
    // printf("Float %f: %d %d %x (scale %d) => %d\n", f, sign, exponent, mantissa, scale, result);
    return result;
}

static void mult(microbit_audio_frame_obj_t *self, float f) {
    int scaled = float_to_fixed(f, 15);
    for (int i = 0; i < self->size; i++) {
        unsigned val = ((((int)self->data[i]-128) * scaled) >> 15)+128;
        if (val > 255) {
            val = (1-(val>>31))*255;
        }
        self->data[i] = val;
    }
}

static mp_obj_t audio_frame_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    if (mp_obj_get_type(lhs_in) != &microbit_audio_frame_type) {
        return MP_OBJ_NULL; // op not supported
    }
    microbit_audio_frame_obj_t *lhs = (microbit_audio_frame_obj_t *)lhs_in;
    switch(op) {
        case MP_BINARY_OP_ADD:
        case MP_BINARY_OP_SUBTRACT:
            lhs = copy(lhs);
        case MP_BINARY_OP_INPLACE_ADD:
        case MP_BINARY_OP_INPLACE_SUBTRACT:
            if (mp_obj_get_type(rhs_in) != &microbit_audio_frame_type) {
                return MP_OBJ_NULL; // op not supported
            }
            add_into(lhs, (microbit_audio_frame_obj_t *)rhs_in, op==MP_BINARY_OP_ADD||op==MP_BINARY_OP_INPLACE_ADD);
            return lhs;
        case MP_BINARY_OP_MULTIPLY:
            lhs = copy(lhs);
        case MP_BINARY_OP_INPLACE_MULTIPLY:
            mult(lhs, mp_obj_get_float(rhs_in));
            return lhs;
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static const mp_map_elem_t microbit_audio_frame_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_copyfrom), (mp_obj_t)&copyfrom_obj },
};
static MP_DEFINE_CONST_DICT(microbit_audio_frame_locals_dict, microbit_audio_frame_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    microbit_audio_frame_type,
    MP_QSTR_AudioFrame,
    MP_TYPE_FLAG_NONE,
    make_new, microbit_audio_frame_new,
    unary_op, audio_frame_unary_op,
    binary_op, audio_frame_binary_op,
    subscr, audio_frame_subscr,
    buffer, audio_frame_get_buffer,
    locals_dict, &microbit_audio_frame_locals_dict
    );

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(size_t size) {
    microbit_audio_frame_obj_t *res = m_new_obj_var(microbit_audio_frame_obj_t, uint8_t, size);
    res->base.type = &microbit_audio_frame_type;
    res->size = size;
    memset(res->data, 128, size);
    return res;
}

MP_REGISTER_ROOT_POINTER(struct _microbit_audio_frame_obj_t *audio_source_frame_state);
MP_REGISTER_ROOT_POINTER(mp_obj_t audio_source_iter_state);
