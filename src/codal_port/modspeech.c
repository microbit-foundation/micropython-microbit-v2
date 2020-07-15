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

#include <stdio.h>

#include "py/obj.h"
#include "py/objtuple.h"
#include "py/objstr.h"
#include "modaudio.h"
#include "sam/reciter.h"
#include "sam/sam.h"

#define SCALE_RATE(x) (((x) * 420) >> 15)

typedef struct _speech_iterator_t {
    mp_obj_base_t base;
    microbit_audio_frame_obj_t *buf;
    microbit_audio_frame_obj_t *empty;
} speech_iterator_t;

extern int debug;

static int synth_mode = 0;
static microbit_audio_frame_obj_t *buf;
static volatile unsigned int buf_start_pos = 0;
static volatile unsigned int last_pos = 0;
static volatile unsigned int last_idx = 0;
static unsigned char last_b = 0;
volatile bool rendering = false;
volatile bool last_frame = false;
volatile bool exhausted = false;
static unsigned int glitches;

// Called by SAM to output byte `b` at `pos`
void SamOutputByte(unsigned int pos, unsigned char b) {
    // b is 4-bit, so make it 8-bit to amplify it a little
    b |= b >> 4;

    if (synth_mode == 0) {
        // Traditional micro:bit v1

        unsigned int actual_pos = SCALE_RATE(pos);
        if (buf_start_pos > actual_pos) {
            glitches++;
            buf_start_pos -= 32;
        }
        while ((actual_pos & (-32)) > buf_start_pos) {
            // We have filled buffer
            rendering = true;
            mp_handle_pending(true);
        }
        unsigned int offset = actual_pos & 31;
        // write a little bit in advance
        unsigned int end = MIN(offset+8, 32);
        while (offset < end) {
            buf->data[offset] = b;
            offset++;
        }
        last_pos = actual_pos;
    } else {
        unsigned int idx_full;
        if (synth_mode == 1 || synth_mode == 2) {
            // coarse
            idx_full = pos >> 6;
        } else {
            // more fidelity
            idx_full = pos >> 5;
        }
        if (buf_start_pos > idx_full) {
            glitches++;
            buf_start_pos -= 32;
        }

        unsigned int idx = idx_full & 31;
        unsigned int delta_idx = (idx - last_idx) & 31;
        if (delta_idx > 0) {
            int cur_b = last_b;
            int delta_b = ((int)b - (int)last_b) / (int)delta_idx;
            while (last_idx != idx) {
                ++last_idx;
                if (last_idx >= 32) {
                    // filled the buffer, wait for it to be drained
                    while ((idx_full & (-32)) > buf_start_pos) {
                        rendering = true;
                        mp_handle_pending(true);
                    }
                    last_idx = 0;
                }
                if (last_idx == idx) {
                    cur_b = b;
                } else {
                    cur_b += delta_b;
                }
                if (synth_mode == 1 || synth_mode == 3) {
                    // no smoothing
                    buf->data[last_idx] = b;
                } else {
                    // smoothing
                    buf->data[last_idx] = cur_b;
                }
            }
        }
        last_idx = idx;
        last_b = b;
    }
}

// This iterator assumes that the speech renderer can generate samples
// at least as fast as we can consume them.
STATIC mp_obj_t next(mp_obj_t iter) {
    if (exhausted) {
        return MP_OBJ_STOP_ITERATION;
    }
    if (last_frame) {
        exhausted = true;
        last_frame = false;
    }
    // May need to wait for reciter to do its job before renderer generate samples.
    if (rendering) {
        buf_start_pos += 32;
        return buf;
    } else {
        return ((speech_iterator_t *)iter)->empty;
    }
}

STATIC const mp_obj_type_t speech_iterator_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .getiter = mp_identity_getiter,
    .iternext = next,
};

STATIC mp_obj_t make_speech_iter(void) {
    speech_iterator_t *result = m_new_obj(speech_iterator_t);
    result->base.type = &speech_iterator_type;
    result->empty = microbit_audio_frame_make_new();
    result->buf = microbit_audio_frame_make_new();
    return result;
}

STATIC mp_obj_t translate(mp_obj_t words) {
    mp_uint_t len, outlen;
    const char *txt = mp_obj_str_get_data(words, &len);
    // Reciter truncates *output* at about 120 characters.
    // So to avoid that we must disallow any input that will exceed that.
    if (len > 80) {
        mp_raise_ValueError("text too long");
    }
    reciter_memory *mem = m_new(reciter_memory, 1);
    MP_STATE_PORT(speech_data) = mem;
    for (mp_uint_t i = 0; i < len; i++) {
        mem->input[i] = txt[i];
    }
    mem->input[len] = '[';
    if (!TextToPhonemes(mem)) {
        MP_STATE_PORT(speech_data) = NULL;
        mp_raise_ValueError("could not parse input");
    }
    for (outlen = 0; outlen < 255; outlen++) {
        if (mem->input[outlen] == 155) {
            break;
        }
    }
    mp_obj_t res = mp_obj_new_str_of_type(&mp_type_str, (byte *)mem->input, outlen);
    // Prevent input becoming invisible to GC due to tail-call optimisation.
    MP_STATE_PORT(speech_data) = NULL;
    return res;
}
MP_DEFINE_CONST_FUN_OBJ_1(translate_obj, translate);

STATIC mp_obj_t articulate(mp_obj_t phonemes, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args, bool sing) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pitch,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_PITCH} },
        { MP_QSTR_speed,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_SPEED} },
        { MP_QSTR_mouth,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_MOUTH} },
        { MP_QSTR_throat,   MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_THROAT} },
        { MP_QSTR_debug,   MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    sam_memory *sam = m_new(sam_memory, 1);
    MP_STATE_PORT(speech_data) = sam;

    // set the current saved speech state
    sam->common.singmode = sing;
    sam->common.pitch  = args[0].u_int;
    sam->common.speed  = args[1].u_int;
    sam->common.mouth  = args[2].u_int;
    sam->common.throat = args[3].u_int;
    debug = args[4].u_bool;
    synth_mode = args[5].u_int;

    mp_uint_t len;
    const char *input = mp_obj_str_get_data(phonemes, &len);
    buf_start_pos = 0;
    speech_iterator_t *src = make_speech_iter();
    buf = src->buf;
    /* We need to wait for reciter to do its job */
    rendering = false;
    exhausted = false;
    glitches = 0;
    int sample_rate;
    if (synth_mode == 0) {
        sample_rate = 15625;
    } else {
        sample_rate = 19000;
        if (synth_mode >= 3) {
            sample_rate *= 2;
        }
    }
    microbit_audio_play_source(src, mp_const_none, mp_const_none, false, sample_rate);

    SetInput(sam, input, len);
    if (!SAMMain(sam)) {
        microbit_audio_stop();
        MP_STATE_PORT(speech_data) = NULL;
        mp_raise_ValueError(sam_error);
    }

    last_frame = true;
    /* Wait for audio finish before returning */
    while (microbit_audio_is_playing()) {
        mp_handle_pending(true);
    }
    MP_STATE_PORT(speech_data) = NULL;
    if (debug) {
        printf("Glitches: %d\r\n", glitches);
    }
    return mp_const_none;
}

STATIC mp_obj_t say(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_obj_t phonemes = translate(pos_args[0]);
    return articulate(phonemes, n_args-1, pos_args+1, kw_args, false);
}
MP_DEFINE_CONST_FUN_OBJ_KW(say_obj, 1, say);

STATIC mp_obj_t pronounce(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    return articulate(pos_args[0], n_args-1, pos_args+1, kw_args, false);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pronounce_obj, 1, pronounce);

STATIC mp_obj_t sing(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    return articulate(pos_args[0], n_args-1, pos_args+1, kw_args, true);
}
MP_DEFINE_CONST_FUN_OBJ_KW(sing_obj, 1, sing);

STATIC const mp_map_elem_t _globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_speech) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_say), (mp_obj_t)&say_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sing), (mp_obj_t)&sing_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_pronounce), (mp_obj_t)&pronounce_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_translate), (mp_obj_t)&translate_obj },
};
STATIC MP_DEFINE_CONST_DICT(_globals, _globals_table);

const mp_obj_module_t speech_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&_globals,
};
