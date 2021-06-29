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

#include <string.h>

#include "nrf_gpio.h"

#include "py/mphal.h"
#include "drv_system.h"
#include "modaudio.h"
#include "modmicrobit.h"

#define audio_source_iter MP_STATE_PORT(audio_source)

#define DEFAULT_SAMPLE_RATE (7812)
#define BUFFER_EXPANSION (4) // smooth out the samples via linear interpolation
#define OUT_CHUNK_SIZE (BUFFER_EXPANSION * AUDIO_CHUNK_SIZE)

static volatile bool audio_running = false;
static uint8_t audio_output_buffer[OUT_CHUNK_SIZE];
static volatile int audio_output_read;
static volatile bool audio_fetecher_scheduled;

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(void);

void microbit_audio_stop(void) {
    audio_source_iter = NULL;
    audio_running = false;
}

STATIC void audio_buffer_ready(void) {
    uint32_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    int x = audio_output_read;
    audio_output_read = 0;
    MICROPY_END_ATOMIC_SECTION(atomic_state);
    if (x == -2) {
        microbit_hal_audio_ready_callback();
    }
}

STATIC void audio_data_fetcher(void) {
    audio_fetecher_scheduled = false;
    if (audio_source_iter == NULL) {
        microbit_audio_stop();
        return;
    }
    mp_obj_t buffer_obj;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        buffer_obj = mp_iternext_allow_raise(audio_source_iter);
        nlr_pop();
    } else {
        if (!mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t*)nlr.ret_val)->type),
            MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
            mp_sched_exception(MP_OBJ_FROM_PTR(nlr.ret_val));
        }
        buffer_obj = MP_OBJ_STOP_ITERATION;
    }
    if (buffer_obj == MP_OBJ_STOP_ITERATION) {
        // End of audio iterator
        audio_source_iter = NULL;
        microbit_audio_stop();
    } else if (mp_obj_get_type(buffer_obj) != &microbit_audio_frame_type) {
        // Audio iterator did not return an AudioFrame
        audio_source_iter = NULL;
        microbit_audio_stop();
        mp_sched_exception(mp_obj_new_exception_msg(&mp_type_TypeError, MP_ERROR_TEXT("not an AudioFrame")));
    } else {
        microbit_audio_frame_obj_t *buffer = (microbit_audio_frame_obj_t *)buffer_obj;
        uint8_t *dest = &audio_output_buffer[0];
        uint32_t last = dest[BUFFER_EXPANSION * AUDIO_CHUNK_SIZE - 1];
        for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i) {
            uint32_t cur = buffer->data[i];
            for (int j = 0; j < BUFFER_EXPANSION; ++j) {
                // Get next sample with linear interpolation.
                uint32_t sample = ((BUFFER_EXPANSION - 1 - j) * last + (j + 1) * cur) / BUFFER_EXPANSION;
                // Write sample to the buffer.
                *dest++ = sample;
            }
            last = cur;
        }
        audio_buffer_ready();
    }
}

STATIC mp_obj_t audio_data_fetcher_wrapper(mp_obj_t arg) {
    audio_data_fetcher();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_data_fetcher_wrapper_obj, audio_data_fetcher_wrapper);

void microbit_hal_audio_ready_callback(void) {
    if (audio_output_read >= 0) {
        // there is data ready to send out to the audio pipeline, so send it
        microbit_hal_audio_write_data(&audio_output_buffer[0], OUT_CHUNK_SIZE);
        audio_output_read = -1;
    } else {
        // no data ready, need to call this function later when data is ready
        audio_output_read = -2;
    }
    if (!audio_fetecher_scheduled) {
        // schedule audio_data_fetcher to be executed to prepare the next buffer
        audio_fetecher_scheduled = mp_sched_schedule(MP_OBJ_FROM_PTR(&audio_data_fetcher_wrapper_obj), mp_const_none);
    }
}

static void audio_init(uint32_t sample_rate) {
    audio_fetecher_scheduled = false;
    audio_output_read = -2;
    microbit_hal_audio_init(BUFFER_EXPANSION * sample_rate);
}

void microbit_audio_play_source(mp_obj_t src, mp_obj_t pin_select, bool wait, uint32_t sample_rate) {
    if (audio_running) {
        microbit_audio_stop();
    }
    audio_init(sample_rate);
    microbit_pin_audio_select(pin_select);

    if (mp_obj_is_type(src, &microbit_sound_type)) {
        const microbit_sound_obj_t *sound = (const microbit_sound_obj_t *)MP_OBJ_TO_PTR(src);
        microbit_hal_audio_play_expression_by_name(sound->name);
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

    // Get the iterator and start the audio running.
    audio_source_iter = mp_getiter(src, NULL);
    audio_running = true;
    audio_data_fetcher();

    if (wait) {
        // Wait the audio to exhaust the iterator.
        while (audio_running) {
            mp_handle_pending(true);
            microbit_hal_idle();
        }
    }
}

STATIC mp_obj_t stop(void) {
    microbit_audio_stop();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_audio_stop_obj, stop);

STATIC mp_obj_t play(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_source, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_wait,  MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_pin,   MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&microbit_pin_default_audio_obj)} },
        { MP_QSTR_return_pin,   MP_ARG_OBJ, {.u_obj = mp_const_none } },
    };
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // The return_pin argument from micro:bit v1 is no longer supported.
    if (args[3].u_obj != mp_const_none) {
        mp_raise_ValueError(MP_ERROR_TEXT("return_pin not supported"));
    }

    mp_obj_t src = args[0].u_obj;
    microbit_audio_play_source(src, args[2].u_obj, args[1].u_bool, DEFAULT_SAMPLE_RATE);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_audio_play_obj, 0, play);

bool microbit_audio_is_playing(void) {
    return audio_running;
}

mp_obj_t is_playing(void) {
    return mp_obj_new_bool(audio_running);
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_audio_is_playing_obj, is_playing);

STATIC const mp_rom_map_elem_t audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&microbit_audio_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&microbit_audio_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_playing), MP_ROM_PTR(&microbit_audio_is_playing_obj) },
    { MP_ROM_QSTR(MP_QSTR_AudioFrame), MP_ROM_PTR(&microbit_audio_frame_type) },
};
STATIC MP_DEFINE_CONST_DICT(audio_module_globals, audio_globals_table);

const mp_obj_module_t audio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&audio_module_globals,
};

/******************************************************************************/
// AudioFrame class

STATIC mp_obj_t microbit_audio_frame_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    (void)args;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    return microbit_audio_frame_make_new();
}

STATIC mp_obj_t audio_frame_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value_in) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    mp_int_t index = mp_obj_get_int(index_in);
    if (index < 0 || index >= AUDIO_CHUNK_SIZE) {
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
    (void)self_in;
    switch (op) {
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(32);
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_int_t audio_frame_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    bufinfo->buf = self->data;
    bufinfo->len = AUDIO_CHUNK_SIZE;
    bufinfo->typecode = 'b';
    return 0;
}

static void add_into(microbit_audio_frame_obj_t *self, microbit_audio_frame_obj_t *other, bool add) {
    int mult = add ? 1 : -1;
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        unsigned val = (int)self->data[i] + mult*(other->data[i]-128);
        // Clamp to 0-255
        if (val > 255) {
            val = (1-(val>>31))*255;
        }
        self->data[i] = val;
    }
}

static microbit_audio_frame_obj_t *copy(microbit_audio_frame_obj_t *self) {
    microbit_audio_frame_obj_t *result = microbit_audio_frame_make_new();
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        result->data[i] = self->data[i];
    }
    return result;
}

mp_obj_t copyfrom(mp_obj_t self_in, mp_obj_t other) {
    microbit_audio_frame_obj_t *self = (microbit_audio_frame_obj_t *)self_in;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(other, &bufinfo, MP_BUFFER_READ);
    uint32_t len = bufinfo.len > AUDIO_CHUNK_SIZE ? AUDIO_CHUNK_SIZE : bufinfo.len;
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
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        unsigned val = ((((int)self->data[i]-128) * scaled) >> 15)+128;
        if (val > 255) {
            val = (1-(val>>31))*255;
        }
        self->data[i] = val;
    }
}

STATIC mp_obj_t audio_frame_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
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

STATIC const mp_map_elem_t microbit_audio_frame_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_copyfrom), (mp_obj_t)&copyfrom_obj },
};
STATIC MP_DEFINE_CONST_DICT(microbit_audio_frame_locals_dict, microbit_audio_frame_locals_dict_table);

const mp_obj_type_t microbit_audio_frame_type = {
    { &mp_type_type },
    .name = MP_QSTR_AudioFrame,
    .make_new = microbit_audio_frame_new,
    .unary_op = audio_frame_unary_op,
    .binary_op = audio_frame_binary_op,
    .subscr = audio_frame_subscr,
    .buffer_p = { .get_buffer = audio_frame_get_buffer },
    .locals_dict = (mp_obj_dict_t*)&microbit_audio_frame_locals_dict,
};

microbit_audio_frame_obj_t *microbit_audio_frame_make_new(void) {
    microbit_audio_frame_obj_t *res = m_new_obj(microbit_audio_frame_obj_t);
    res->base.type = &microbit_audio_frame_type;
    memset(res->data, 128, AUDIO_CHUNK_SIZE);
    return res;
}
