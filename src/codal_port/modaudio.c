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

#include <math.h>
#include "py/mphal.h"
#include "drv_system.h"
#include "modaudio.h"
#include "modmicrobit.h"

// Convenience macros to access the root-pointer state.
#define audio_source_frame MP_STATE_PORT(audio_source_frame_state)
#define audio_source_track MP_STATE_PORT(audio_source_track_state)
#define audio_source_iter MP_STATE_PORT(audio_source_iter_state)

#ifndef AUDIO_OUTPUT_BUFFER_SIZE
#define AUDIO_OUTPUT_BUFFER_SIZE (32)
#endif

#define DEFAULT_AUDIO_FRAME_SIZE (32)
#define DEFAULT_SAMPLE_RATE (7812)

typedef enum {
    AUDIO_OUTPUT_STATE_IDLE,
    AUDIO_OUTPUT_STATE_DATA_READY,
    AUDIO_OUTPUT_STATE_DATA_WRITTEN,
} audio_output_state_t;

static uint8_t audio_output_buffer[AUDIO_OUTPUT_BUFFER_SIZE];
static size_t audio_output_buffer_offset;
static volatile audio_output_state_t audio_output_state;
static size_t audio_source_frame_offset;
static uint32_t audio_current_sound_level;
static mp_sched_node_t audio_data_fetcher_sched_node;

static inline bool audio_is_running(void) {
    return audio_source_frame != NULL || audio_source_track != NULL || audio_source_iter != MP_OBJ_NULL;
}

void microbit_audio_stop(void) {
    audio_output_buffer_offset = 0;
    audio_source_frame = NULL;
    audio_source_track = NULL;
    audio_source_iter = NULL;
    audio_source_frame_offset = 0;
    audio_current_sound_level = 0;
    microbit_hal_audio_stop_expression();
}

static void audio_data_pull_from_source(void) {
    if (audio_source_frame != NULL) {
        // An existing AudioFrame is being played, see if there's any data left.
        if (audio_source_frame_offset >= audio_source_frame->alloc_size) {
            // AudioFrame is exhausted.
            audio_source_frame = NULL;
        }
    } else if (audio_source_track != NULL) {
        // An existing AudioTrack is being played, see if there's any data left.
        if (audio_source_frame_offset >= audio_source_track->size) {
            // AudioTrack is exhausted.
            audio_source_track = NULL;
        }
    }

    if (audio_source_frame == NULL && audio_source_track == NULL) {
        // There is no AudioFrame/AudioTrack, so try to get one from the audio iterator.

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

        if (mp_obj_is_type(frame_obj, &microbit_audio_frame_type)) {
            // We have the next AudioFrame.
            audio_source_frame = MP_OBJ_TO_PTR(frame_obj);
            audio_source_frame_offset = 0;
            microbit_hal_audio_raw_set_rate(audio_source_frame->rate);
        } else if (mp_obj_is_type(frame_obj, &microbit_audio_track_type)
            || mp_obj_is_type(frame_obj, &microbit_audio_recording_type)) {
            // We have the next AudioTrack/AudioRecording.
            audio_source_track = MP_OBJ_TO_PTR(frame_obj);
            audio_source_frame_offset = 0;
            microbit_hal_audio_raw_set_rate(audio_source_track->rate);
        } else {
            // Audio iterator did not return an AudioFrame/AudioTrack/AudioRecording.
            microbit_audio_stop();
            mp_sched_exception(mp_obj_new_exception_msg(&mp_type_TypeError, MP_ERROR_TEXT("not an AudioFrame")));
            return;
        }
    }
}

static void audio_data_fetcher(mp_sched_node_t *node) {
    audio_data_pull_from_source();
    uint8_t *dest = &audio_output_buffer[audio_output_buffer_offset];

    if (audio_source_frame == NULL && audio_source_track == NULL) {
        // Audio source is exhausted.

        if (audio_output_buffer_offset == 0) {
            // No output data left, finish output streaming.
            return;
        }

        // Fill remaining audio_output_buffer bytes with silence, for the final output frame.
        memset(dest, 128, AUDIO_OUTPUT_BUFFER_SIZE - audio_output_buffer_offset);
        audio_output_buffer_offset = AUDIO_OUTPUT_BUFFER_SIZE;
    } else {
        // Copy samples to the buffer.
        const uint8_t *src;
        size_t size;
        if (audio_source_frame != NULL) {
            src = &audio_source_frame->data[audio_source_frame_offset];
            size = audio_source_frame->alloc_size;
        } else {
            src = &audio_source_track->data[audio_source_frame_offset];
            size = audio_source_track->size;
        }
        size_t src_len = MIN(size - audio_source_frame_offset, AUDIO_OUTPUT_BUFFER_SIZE - audio_output_buffer_offset);
        memcpy(dest, src, src_len);

        // Update output and source offsets.
        audio_output_buffer_offset += src_len;
        audio_source_frame_offset += src_len;
    }

    if (audio_output_buffer_offset < AUDIO_OUTPUT_BUFFER_SIZE) {
        // Output buffer not full yet, so attempt to pull more data from the source.
        mp_sched_schedule_node(&audio_data_fetcher_sched_node, audio_data_fetcher);
    } else {
        // Output buffer is full, process it and prepare for next buffer fill.
        audio_output_buffer_offset = 0;

        // Compute the sound level.
        uint32_t sound_level = 0;
        for (int i = 0; i < AUDIO_OUTPUT_BUFFER_SIZE; ++i) {
            sound_level += (audio_output_buffer[i] - 128) * (audio_output_buffer[i] - 128);
        }
        audio_current_sound_level = sound_level / AUDIO_OUTPUT_BUFFER_SIZE;

        // Send the data to the lower levels of the audio pipeline.
        uint32_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
        audio_output_state_t old_state = audio_output_state;
        audio_output_state = AUDIO_OUTPUT_STATE_DATA_READY;
        MICROPY_END_ATOMIC_SECTION(atomic_state);
        if (old_state == AUDIO_OUTPUT_STATE_IDLE) {
            microbit_hal_audio_raw_ready_callback();
        }
    }
}

void microbit_hal_audio_raw_ready_callback(void) {
    if (audio_output_state == AUDIO_OUTPUT_STATE_DATA_READY) {
        // there is data ready to send out to the audio pipeline, so send it
        microbit_hal_audio_raw_write_data(&audio_output_buffer[0], AUDIO_OUTPUT_BUFFER_SIZE);
        audio_output_state = AUDIO_OUTPUT_STATE_DATA_WRITTEN;
    } else {
        // no data ready, need to call this function later when data is ready
        audio_output_state = AUDIO_OUTPUT_STATE_IDLE;
    }

    // schedule audio_data_fetcher to be executed to prepare the next buffer
    mp_sched_schedule_node(&audio_data_fetcher_sched_node, audio_data_fetcher);
}

static void audio_init(uint32_t sample_rate) {
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
        audio_source_frame_offset = 0;
        microbit_hal_audio_raw_set_rate(audio_source_frame->rate);
    } else if (mp_obj_is_type(src, &microbit_audio_track_type)
        || mp_obj_is_type(src, &microbit_audio_recording_type)) {
        audio_source_track = MP_OBJ_TO_PTR(src);
        audio_source_frame_offset = 0;
        microbit_hal_audio_raw_set_rate(audio_source_track->rate);
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
    audio_data_fetcher(&audio_data_fetcher_sched_node);
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

// Returns a number between 0 and 254, being the average intensity of the sound played
// from the most recent chunk of data.
static mp_obj_t microbit_audio_sound_level(void) {
    return MP_OBJ_NEW_SMALL_INT(2 * sqrt(audio_current_sound_level));
}
static MP_DEFINE_CONST_FUN_OBJ_0(microbit_audio_sound_level_obj, microbit_audio_sound_level);

static const mp_rom_map_elem_t audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&microbit_audio_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&microbit_audio_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_playing), MP_ROM_PTR(&microbit_audio_is_playing_obj) },
    { MP_ROM_QSTR(MP_QSTR_sound_level), MP_ROM_PTR(&microbit_audio_sound_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_AudioFrame), MP_ROM_PTR(&microbit_audio_frame_type) },
    { MP_ROM_QSTR(MP_QSTR_AudioRecording), MP_ROM_PTR(&microbit_audio_recording_type) },
    { MP_ROM_QSTR(MP_QSTR_AudioTrack), MP_ROM_PTR(&microbit_audio_track_type) },
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

static mp_obj_t microbit_audio_frame_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    (void)type_in;

    enum { ARG_duration, ARG_rate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_duration, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_rate, MP_ARG_INT, {.u_int = DEFAULT_SAMPLE_RATE} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t rate = args[ARG_rate].u_int;
    if (rate <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("rate out of bounds"));
    }

    size_t size;
    if (args[ARG_duration].u_obj == mp_const_none) {
        size = DEFAULT_AUDIO_FRAME_SIZE;
    } else {
        mp_float_t duration = mp_obj_get_float(args[ARG_duration].u_obj);
        if (duration <= 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("size out of bounds"));
        }
        size = duration * rate / 1000;
    }

    return microbit_audio_frame_make_new(size, rate);
}

static mp_obj_t audio_frame_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value_in) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    mp_int_t index = mp_obj_get_int(index_in);
    if (index < 0 || index >= self->alloc_size) {
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
            return MP_OBJ_NEW_SMALL_INT(self->alloc_size);
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_int_t audio_frame_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    bufinfo->buf = self->data;
    bufinfo->len = self->alloc_size;
    bufinfo->typecode = 'B';
    return 0;
}

void microbit_audio_data_add_inplace(uint8_t *lhs_data, const uint8_t *rhs_data, size_t size, bool add) {
    int mult = add ? 1 : -1;
    for (int i = 0; i < size; i++) {
        unsigned val = (int)lhs_data[i] + mult*(rhs_data[i]-128);
        // Clamp to 0-255
        if (val > 255) {
            val = (1-(val>>31))*255;
        }
        lhs_data[i] = val;
    }
}

static microbit_audio_frame_obj_t *copy(microbit_audio_frame_obj_t *self) {
    microbit_audio_frame_obj_t *result = microbit_audio_frame_make_new(self->alloc_size, self->rate);
    for (int i = 0; i < self->alloc_size; i++) {
        result->data[i] = self->data[i];
    }
    return result;
}

mp_obj_t copyfrom(mp_obj_t self_in, mp_obj_t other) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(other, &bufinfo, MP_BUFFER_READ);
    uint32_t len = MIN(bufinfo.len, self->alloc_size);
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

void microbit_audio_data_mult_inplace(uint8_t *data, size_t size, float f) {
    int scaled = float_to_fixed(f, 15);
    for (int i = 0; i < size; i++) {
        unsigned val = ((((int)data[i]-128) * scaled) >> 15)+128;
        if (val > 255) {
            val = (1-(val>>31))*255;
        }
        data[i] = val;
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
            microbit_audio_frame_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
            size_t size = MIN(lhs->alloc_size, rhs->alloc_size);
            microbit_audio_data_add_inplace(lhs->data, rhs->data, size, op == MP_BINARY_OP_ADD || op == MP_BINARY_OP_INPLACE_ADD);
            return lhs;
        case MP_BINARY_OP_MULTIPLY:
            lhs = copy(lhs);
        case MP_BINARY_OP_INPLACE_MULTIPLY:
            microbit_audio_data_mult_inplace(lhs->data, lhs->alloc_size, mp_obj_get_float(rhs_in));
            return lhs;
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_obj_t audio_frame_get_rate(mp_obj_t self_in) {
    microbit_audio_frame_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->rate);
}
static MP_DEFINE_CONST_FUN_OBJ_1(audio_frame_get_rate_obj, audio_frame_get_rate);

static mp_obj_t audio_frame_set_rate(mp_obj_t self_in, mp_obj_t rate_in) {
    microbit_audio_frame_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rate = mp_obj_get_int(rate_in);
    if (rate <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("rate out of bounds"));
    }
    self->rate = rate;
    // TODO: only set if this frame is currently being played
    microbit_hal_audio_raw_set_rate(rate);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(audio_frame_set_rate_obj, audio_frame_set_rate);

static const mp_map_elem_t microbit_audio_frame_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_rate), (mp_obj_t)&audio_frame_get_rate_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_rate), (mp_obj_t)&audio_frame_set_rate_obj },
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

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(size_t size, uint32_t rate) {
    // Make sure size is non-zero.
    if (size == 0) {
        size = 1;
    }

    microbit_audio_frame_obj_t *res = m_new_obj_var(microbit_audio_frame_obj_t, data, uint8_t, size);
    res->base.type = &microbit_audio_frame_type;
    res->alloc_size = size;
    res->rate = rate;
    memset(res->data, 128, size);
    return res;
}

MP_REGISTER_ROOT_POINTER(struct _microbit_audio_frame_obj_t *audio_source_frame_state);
MP_REGISTER_ROOT_POINTER(struct _microbit_audio_track_obj_t *audio_source_track_state);
MP_REGISTER_ROOT_POINTER(mp_obj_t audio_source_iter_state);
