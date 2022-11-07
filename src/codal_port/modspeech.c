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
#include <string.h>

#include "py/obj.h"
#include "py/objtuple.h"
#include "py/objstr.h"
#include "microbithal.h"
#include "modmicrobit.h"
#include "modaudio.h"
#include "sam/reciter.h"
#include "sam/sam.h"

// If disabled, pipe speech through audio module output.
// If enabled, use a dedicated audio mixer channer with a double buffer.
#define USE_DEDICATED_AUDIO_CHANNEL (1)

#if USE_DEDICATED_AUDIO_CHANNEL
#define OUT_CHUNK_SIZE (128)
#else
#define OUT_CHUNK_SIZE (32) // must match audio frame size
#endif

#define SCALE_RATE(x) (((x) * 420) >> 15)

typedef struct _speech_iterator_t {
    mp_obj_base_t base;
    microbit_audio_frame_obj_t *buf;
    microbit_audio_frame_obj_t *empty;
} speech_iterator_t;

extern int debug;

static int synth_mode = 0;
static int synth_volume = 0;
static microbit_audio_frame_obj_t *sam_output_frame;
static volatile unsigned int buf_start_pos = 0;
static volatile unsigned int last_pos = 0;
static volatile unsigned int last_idx = 0;
static unsigned char last_b = 0;
volatile bool rendering = false;
volatile bool last_frame = false;
volatile bool exhausted = false;
static unsigned int glitches;

#if USE_DEDICATED_AUDIO_CHANNEL
static uint8_t speech_output_buffer[2 * OUT_CHUNK_SIZE];
static unsigned int speech_output_buffer_idx;
static volatile int speech_output_write;
static volatile int speech_output_read;
#else
static volatile bool audio_output_ready = false;
#endif

void microbit_hal_audio_speech_ready_callback(void) {
    #if USE_DEDICATED_AUDIO_CHANNEL
    if (speech_output_read >= 0) {
        microbit_hal_audio_speech_write_data(&speech_output_buffer[OUT_CHUNK_SIZE * speech_output_read], OUT_CHUNK_SIZE);
        speech_output_read = -1;
    } else {
        // missed
        speech_output_read = -2;
    }
    #else
    audio_output_ready = true;
    #endif
}

STATIC void sam_output_reset(microbit_audio_frame_obj_t *src_frame) {
    sam_output_frame = src_frame;
    buf_start_pos = 0;
    last_pos = 0;
    last_idx = 0;
    last_b = 0;
    rendering = false;
    last_frame = false;
    exhausted = false;
    glitches = 0;
    #if USE_DEDICATED_AUDIO_CHANNEL
    speech_output_buffer_idx = 0;
    speech_output_write = 0;
    speech_output_read = -2;
    #else
    audio_output_ready = true;
    #endif
}

STATIC void speech_wait_output_drained(void) {
    #if USE_DEDICATED_AUDIO_CHANNEL
    while (speech_output_read >= 0) {
        mp_handle_pending(true);
        extern void microbit_hal_background_processing(void);
        microbit_hal_background_processing();
    }
    uint32_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    int x = speech_output_read;
    speech_output_read = speech_output_write;
    MICROPY_END_ATOMIC_SECTION(atomic_state);
    speech_output_write = 1 - speech_output_write;
    if (x == -2) {
        microbit_hal_audio_speech_ready_callback();
    }
    #else
    rendering = true;
    mp_handle_pending(true);
    extern void microbit_hal_background_processing(void);
    microbit_hal_background_processing();
    #endif
}

#if USE_DEDICATED_AUDIO_CHANNEL
STATIC void speech_output_sample(uint8_t b) {
    speech_output_buffer[OUT_CHUNK_SIZE * speech_output_write + speech_output_buffer_idx++] = b;
    if (speech_output_buffer_idx >= OUT_CHUNK_SIZE) {
        speech_wait_output_drained();
        speech_output_buffer_idx = 0;
    }
}
#endif

// Table to map SAM value `b>>4` to an output value for the PWM.
// This tries to maximise output volume with minimal distortion.
static const uint8_t sam_sample_remap[16] = {
    [0] = 0, // 0
    [1] = 1, // 16
    [2] = 2, // 32
    [3] = 4, // 48
    [4] = 8, // 64
    [5] = 16, // 80
    [6] = 32, // 96
    [7] = 64, // 112
    [8] = 128, // 128
    [9] = 192, // 144
    [10] = 224, // 160
    [11] = 240, // 176
    [12] = 248, // 192
    [13] = 252, // 208
    [14] = 254, // 224
    [15] = 255, // 240
};

// Called by SAM to output byte `b` at `pos`
// b is a value between 0 and 240 and a multiple of 16.
//
// Typical histogram of values for `b>>4` for some text:
// b>>4  number
//  0    0
//  1    3
//  2    31
//  3    165
//  4    536
//  5    7016
//  6    2859
//  7    6426
//  8    13549
//  9    4092
// 10    5573
// 11    987
// 12    594
// 13    254
// 14    82
// 15    1
void SamOutputByte(unsigned int pos, unsigned char b) {
    // Adjust b to increase volume, based on synth_volume setting.
    if (synth_volume == 0) {
        // pass
    } else if (synth_volume == 1) {
        b |= b >> 4;
    } else if (synth_volume == 2) {
        if (b < (2 << 4)) b = 2 << 4;
        if (b > (14 << 4)) b = 14 << 4;
        b = ((uint32_t)b - (2 << 4)) * 255 / (12 << 4);
    } else if (synth_volume == 3) {
        if (b < (3 << 4)) b = 3 << 4;
        if (b > (13 << 4)) b = 13 << 4;
        b = ((uint32_t)b - (3 << 4)) * 255 / (10 << 4);
    } else if (synth_volume == 4) {
        b = sam_sample_remap[b >> 4];
    }

    if (synth_mode == 0) {
        // Traditional micro:bit v1

        #if USE_DEDICATED_AUDIO_CHANNEL
        // Not supported.
        #else
        unsigned int actual_pos = SCALE_RATE(pos);
        if (buf_start_pos > actual_pos) {
            glitches++;
            buf_start_pos -= OUT_CHUNK_SIZE;
        }
        while ((actual_pos & (-OUT_CHUNK_SIZE)) > buf_start_pos) {
            // We have filled buffer
            speech_wait_output_drained();
        }
        unsigned int offset = actual_pos & (OUT_CHUNK_SIZE - 1);
        // write a little bit in advance
        unsigned int end = MIN(offset+8, OUT_CHUNK_SIZE);
        while (offset < end) {
            sam_output_frame->data[offset] = b;
            offset++;
        }
        last_pos = actual_pos;
        #endif
    } else {
        unsigned int idx_full;
        if (synth_mode == 1 || synth_mode == 2) {
            // coarse
            idx_full = pos >> 6;
        } else {
            // more fidelity
            idx_full = pos >> 5;
        }

        // Need to output sample b at position idx_full.

        #if USE_DEDICATED_AUDIO_CHANNEL

        if (synth_mode == 1 || synth_mode == 3) {
            // No smoothing, just output b as many times as needed to get to idx_full.
            while (last_idx < idx_full) {
                last_idx += 1;
                speech_output_sample(b);
            }
        } else {
            // Apply linear interpolation from last_b to b.
            unsigned int delta_idx = idx_full - last_idx;
            if (delta_idx > 0) {
                int cur_b = last_b;
                int delta_b = ((int)b - (int)last_b) / (int)delta_idx;
                while (last_idx < idx_full) {
                    last_idx += 1;
                    if (last_idx == idx_full) {
                        cur_b = b;
                    } else {
                        cur_b += delta_b;
                    }
                    speech_output_sample(cur_b);
                }
            }
            last_b = b;
        }

        #else

        if (buf_start_pos > idx_full) {
            glitches++;
            buf_start_pos -= OUT_CHUNK_SIZE;
        }

        unsigned int idx = idx_full & (OUT_CHUNK_SIZE - 1);
        unsigned int delta_idx = (idx - last_idx) & (OUT_CHUNK_SIZE - 1);
        if (delta_idx > 0) {
            int cur_b = last_b;
            int delta_b = ((int)b - (int)last_b) / (int)delta_idx;
            while (last_idx != idx) {
                ++last_idx;
                if (last_idx >= OUT_CHUNK_SIZE) {
                    // filled the buffer, wait for it to be drained
                    while ((idx_full & (-OUT_CHUNK_SIZE)) > buf_start_pos) {
                        speech_wait_output_drained();
                    }
                    last_idx = 0;
                }
                if (last_idx == idx) {
                    cur_b = b;
                } else {
                    cur_b += delta_b;
                }
                uint8_t sample;
                if (synth_mode == 1 || synth_mode == 3) {
                    // no smoothing
                    sample = b;
                } else {
                    // smoothing
                    sample = cur_b;
                }
                sam_output_frame->data[last_idx] = sample;
            }
        }
        last_idx = idx;
        last_b = b;

        #endif
    }
}

#if !USE_DEDICATED_AUDIO_CHANNEL

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
        buf_start_pos += OUT_CHUNK_SIZE;
        return sam_output_frame;
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

#endif

STATIC mp_obj_t translate(mp_obj_t words) {
    mp_uint_t len, outlen;
    const char *txt = mp_obj_str_get_data(words, &len);
    // Reciter truncates *output* at about 120 characters.
    // So to avoid that we must disallow any input that will exceed that.
    if (len > 80) {
        mp_raise_ValueError(MP_ERROR_TEXT("text too long"));
    }
    reciter_memory *mem = m_new(reciter_memory, 1);
    MP_STATE_PORT(speech_data) = mem;
    for (mp_uint_t i = 0; i < len; i++) {
        mem->input[i] = txt[i];
    }
    mem->input[len] = '[';
    if (!TextToPhonemes(mem)) {
        MP_STATE_PORT(speech_data) = NULL;
        mp_raise_ValueError(MP_ERROR_TEXT("could not parse input"));
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
        { MP_QSTR_debug,    MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_volume,   MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 4} },
        { MP_QSTR_pin,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&microbit_pin_default_audio_obj)} },
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
    synth_volume = args[6].u_int;

    mp_uint_t len;
    const char *input = mp_obj_str_get_data(phonemes, &len);
    int sample_rate;
    if (synth_mode == 0) {
        sample_rate = 15625;
    } else if (synth_mode <= 2) {
        sample_rate = 19000;
    } else {
        sample_rate = 38000;
    }

    #if USE_DEDICATED_AUDIO_CHANNEL
    sam_output_reset(NULL);
    microbit_pin_audio_select(args[7].u_obj, microbit_pin_mode_audio_play);
    microbit_hal_audio_speech_init(sample_rate);
    #else
    speech_iterator_t *src = make_speech_iter();
    sam_output_reset(src->buf);
    microbit_audio_play_source(src, args[7].u_obj, false, sample_rate);
    #endif

    SetInput(sam, input, len);
    if (!SAMMain(sam)) {
        microbit_audio_stop();
        MP_STATE_PORT(speech_data) = NULL;
        mp_raise_ValueError((mp_rom_error_text_t)sam_error);
    }

    #if USE_DEDICATED_AUDIO_CHANNEL
    // Finish writing out current buffer.
    while (speech_output_buffer_idx != 0) {
        speech_output_sample(128);
    }
    #else
    last_frame = true;
    /* Wait for audio finish before returning */
    while (microbit_audio_is_playing()) {
        mp_handle_pending(true);
        extern void microbit_hal_background_processing(void);
        microbit_hal_background_processing();
    }
    MP_STATE_PORT(speech_data) = NULL;
    #endif

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
